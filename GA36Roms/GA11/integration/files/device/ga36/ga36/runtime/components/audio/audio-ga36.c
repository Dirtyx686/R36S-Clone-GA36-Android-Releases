/* SPDX-License-Identifier: Apache-2.0
 * GA36 A33 primary PCM output. No old vendor C++/PowerManager ABI is used.
 * Initial bring-up exposes only the built-in speaker. No capture/offload/USB.
 */
#define LOG_TAG "GA36Audio"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <cutils/str_parms.h>
#include <log/log.h>

enum { RATE = 44100, FRAME_BYTES = 4, PERIOD = 1024, PERIODS = 4, GAIN = 57 };
struct ga36_device {
    struct audio_hw_device hw;
    pthread_mutex_t lock;
    struct ga36_output *output;
};
struct ga36_output {
    struct audio_stream_out hw;
    pthread_mutex_t lock;
    struct pcm *pcm;
    struct mixer *mixer;
    uint64_t written, base, presented;
};
static struct ga36_output *out(const struct audio_stream *s) {
    return (struct ga36_output *)(uintptr_t)s;
}
static int control(struct mixer *m, const char *name, int value) {
    struct mixer_ctl *c = mixer_get_ctl_by_name(m, name);
    if (!c || !mixer_ctl_get_num_values(c)) return -ENODEV;
    return mixer_ctl_set_value(c, 0, value) < 0 ? -EIO : 0;
}
/* Same sun8iw5 codec controls as the working kernel and card0 device logs.
 * Speaker Function=1 programs the actual DAC path. Volume is applied LAST:
 * opening the speaker path restores the board's pa_vol in the kernel.
 * 57 is a conservative register value within the real 0..63 range, not dB.
 */
static int speaker_on(struct mixer *m) {
    if (control(m, "Audio headphone out", 0) ||
        control(m, "Audio earpiece out", 0) ||
        control(m, "Speaker Function", 1) ||
        control(m, "Audio speaker out", 1) ||
        control(m, "speaker volume control", GAIN)) return -EIO;
    return 0;
}
static int position_locked(struct ga36_output *o, uint64_t *frames, struct timespec *t) {
    unsigned int avail;
    if (!o->pcm || pcm_get_htimestamp(o->pcm, &avail, t)) return -ENODATA;
    unsigned int capacity = pcm_get_buffer_size(o->pcm);
    if (avail > capacity || o->written < capacity - avail) return -ENODATA;
    uint64_t p = o->base + o->written - (capacity - avail);
    if (p < o->presented) return -ENODATA;
    *frames = o->presented = p;
    return 0;
}
static void standby_locked(struct ga36_output *o) {
    uint64_t p; struct timespec t;
    (void)position_locked(o, &p, &t);
    if (o->pcm) pcm_close(o->pcm);
    if (o->mixer) {
        (void)control(o->mixer, "Audio speaker out", 0);
        mixer_close(o->mixer);
    }
    o->pcm = NULL; o->mixer = NULL;
    o->base = o->presented; o->written = 0;
}
static int start_locked(struct ga36_output *o) {
    struct pcm_config c = { .channels=2, .rate=RATE, .period_size=PERIOD,
        .period_count=PERIODS, .format=PCM_FORMAT_S16_LE,
        .start_threshold=PERIOD, .stop_threshold=PERIOD*PERIODS };
    /* The BSP enables/resets the AIF1/DAC clocks in PCM startup. Applying
     * the digital speaker route before that reset loses its register writes.
     * Prepare first; the later tinyalsa write keeps the prepared PCM intact. */
    o->pcm = pcm_open(0, 0, PCM_OUT | PCM_MONOTONIC, &c);
    if (!o->pcm || !pcm_is_ready(o->pcm) || pcm_prepare(o->pcm)) goto fail;
    o->mixer = mixer_open(0);
    if (!o->mixer || speaker_on(o->mixer)) goto fail;
    return 0;
fail:
    ALOGE("real PCM/speaker open failed: %s", o->pcm ? pcm_get_error(o->pcm) : "mixer/route");
    standby_locked(o);
    return -ENODEV;
}
static uint32_t rate(const struct audio_stream *s) { (void)s; return RATE; }
static int set_rate(struct audio_stream *s, uint32_t r) { (void)s; return r==RATE ? 0 : -EINVAL; }
static size_t buffer_size(const struct audio_stream *s) { (void)s; return PERIOD*FRAME_BYTES; }
static audio_channel_mask_t channels(const struct audio_stream *s) { (void)s; return AUDIO_CHANNEL_OUT_STEREO; }
static audio_format_t format(const struct audio_stream *s) { (void)s; return AUDIO_FORMAT_PCM_16_BIT; }
static int set_format(struct audio_stream *s, audio_format_t f) { (void)s; return f==AUDIO_FORMAT_PCM_16_BIT ? 0 : -EINVAL; }
static int standby(struct audio_stream *s) {
    struct ga36_output *o=out(s); pthread_mutex_lock(&o->lock);
    standby_locked(o); pthread_mutex_unlock(&o->lock); return 0;
}
static int stream_dump(const struct audio_stream *s, int fd) {
    struct ga36_output *o=out(s); pthread_mutex_lock(&o->lock);
    dprintf(fd,"GA36 PCM0 speaker: %dHz stereo S16, gain=%d/63 active=%d written=%llu\n",
        RATE,GAIN,!!o->pcm,(unsigned long long)o->written);
    pthread_mutex_unlock(&o->lock); return 0;
}
static audio_devices_t device(const struct audio_stream *s) { (void)s; return AUDIO_DEVICE_OUT_SPEAKER; }
static int set_device(struct audio_stream *s, audio_devices_t d) {
    if (!d) return standby(s);
    return d==AUDIO_DEVICE_OUT_SPEAKER ? 0 : -EINVAL;
}
static int parameters(struct audio_stream *s, const char *kv) {
    struct str_parms *p=str_parms_create_str(kv); if (!p) return -ENOMEM;
    int value, rc=0;
    if (str_parms_get_int(p,AUDIO_PARAMETER_STREAM_ROUTING,&value)>=0) rc=set_device(s,(audio_devices_t)value);
    if (!rc && str_parms_get_int(p,AUDIO_PARAMETER_STREAM_SAMPLING_RATE,&value)>=0 && value!=RATE) rc=-EINVAL;
    if (!rc && str_parms_get_int(p,AUDIO_PARAMETER_STREAM_FORMAT,&value)>=0 && value!=AUDIO_FORMAT_PCM_16_BIT) rc=-EINVAL;
    if (!rc && str_parms_get_int(p,AUDIO_PARAMETER_STREAM_CHANNELS,&value)>=0 && value!=AUDIO_CHANNEL_OUT_STEREO) rc=-EINVAL;
    str_parms_destroy(p); return rc;
}
static char *get_parameters(const struct audio_stream *s, const char *keys) {
    (void)s; char value[128];
    struct str_parms *q=str_parms_create_str(keys), *p=str_parms_create();
    if (!q || !p) { if(q) str_parms_destroy(q); if(p) str_parms_destroy(p); return strdup(""); }
    if (str_parms_get_str(q,AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES,value,sizeof(value))>=0)
        str_parms_add_str(p,AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES,"44100");
    if (str_parms_get_str(q,AUDIO_PARAMETER_STREAM_SUP_CHANNELS,value,sizeof(value))>=0)
        str_parms_add_str(p,AUDIO_PARAMETER_STREAM_SUP_CHANNELS,"AUDIO_CHANNEL_OUT_STEREO");
    if (str_parms_get_str(q,AUDIO_PARAMETER_STREAM_SUP_FORMATS,value,sizeof(value))>=0)
        str_parms_add_str(p,AUDIO_PARAMETER_STREAM_SUP_FORMATS,"AUDIO_FORMAT_PCM_16_BIT");
    char *result=str_parms_to_str(p); str_parms_destroy(q); str_parms_destroy(p); return result;
}
static int effect(const struct audio_stream *s, effect_handle_t e) { (void)s; (void)e; return 0; }
static uint32_t latency(const struct audio_stream_out *s) { (void)s; return PERIOD*PERIODS*1000/RATE; }
static int volume(struct audio_stream_out *s, float l, float r) { (void)s;(void)l;(void)r;return -ENOSYS; }
static ssize_t write_pcm(struct audio_stream_out *s, const void *buf, size_t bytes) {
    if (!bytes) return 0;
    if (!buf || bytes%FRAME_BYTES || bytes>INT_MAX) return -EINVAL;
    struct ga36_output *o=(struct ga36_output *)s; pthread_mutex_lock(&o->lock);
    int rc=o->pcm ? 0 : start_locked(o);
    if (!rc && pcm_write(o->pcm,buf,(unsigned int)bytes)) {
        ALOGE("PCM write failed: %s",pcm_get_error(o->pcm)); standby_locked(o); rc=-EIO;
    }
    if (!rc) o->written+=bytes/FRAME_BYTES;
    pthread_mutex_unlock(&o->lock);
    return rc ? rc : (ssize_t)bytes;
}
static int presentation(const struct audio_stream_out *s, uint64_t *f, struct timespec *t) {
    if (!f || !t) return -EINVAL;
    struct ga36_output *o=(struct ga36_output *)(uintptr_t)s; pthread_mutex_lock(&o->lock);
    int rc=position_locked(o,f,t); pthread_mutex_unlock(&o->lock); return rc;
}
static int render(const struct audio_stream_out *s, uint32_t *frames) {
    if (!frames) return -EINVAL;
    struct ga36_output *o=(struct ga36_output *)(uintptr_t)s;
    struct timespec t; uint64_t f; pthread_mutex_lock(&o->lock);
    int rc=position_locked(o,&f,&t);
    if (!rc) *frames=(uint32_t)(f-o->base); /* render resets on standby; presentation does not */
    pthread_mutex_unlock(&o->lock); return rc;
}
static int timestamp(const struct audio_stream_out *s, int64_t *t) { (void)s; (void)t; return -ENOSYS; }
static int open_output(struct audio_hw_device *hw, audio_io_handle_t h, audio_devices_t d,
        audio_output_flags_t flags, struct audio_config *c, struct audio_stream_out **result, const char *a) {
    (void)h;(void)a;
    if (!c || !result) return -EINVAL;
    *result=NULL;
    if (d!=AUDIO_DEVICE_OUT_SPEAKER || (flags & ~(AUDIO_OUTPUT_FLAG_PRIMARY|AUDIO_OUTPUT_FLAG_DEEP_BUFFER))) return -EINVAL;
    bool valid=(!c->sample_rate || c->sample_rate==RATE) &&
        (!c->channel_mask || c->channel_mask==AUDIO_CHANNEL_OUT_STEREO) &&
        (!c->format || c->format==AUDIO_FORMAT_PCM_16_BIT);
    c->sample_rate=RATE; c->channel_mask=AUDIO_CHANNEL_OUT_STEREO; c->format=AUDIO_FORMAT_PCM_16_BIT;
    if (!valid) return -EINVAL;
    struct ga36_device *dev=(struct ga36_device *)hw; pthread_mutex_lock(&dev->lock);
    if (dev->output) { pthread_mutex_unlock(&dev->lock); return -EBUSY; }
    struct ga36_output *o=calloc(1,sizeof(*o));
    if (!o) { pthread_mutex_unlock(&dev->lock); return -ENOMEM; }
    int err=pthread_mutex_init(&o->lock,NULL);
    if (err) { free(o); pthread_mutex_unlock(&dev->lock); return -err; }
    o->hw.common=(struct audio_stream){ .get_sample_rate=rate,.set_sample_rate=set_rate,
        .get_buffer_size=buffer_size,.get_channels=channels,.get_format=format,.set_format=set_format,
        .standby=standby,.dump=stream_dump,.get_device=device,.set_device=set_device,
        .set_parameters=parameters,.get_parameters=get_parameters,
        .add_audio_effect=effect,.remove_audio_effect=effect };
    o->hw.get_latency=latency; o->hw.set_volume=volume; o->hw.write=write_pcm;
    o->hw.get_render_position=render; o->hw.get_next_write_timestamp=timestamp;
    o->hw.get_presentation_position=presentation;
    dev->output=o; *result=&o->hw; pthread_mutex_unlock(&dev->lock); return 0;
}
static void close_output(struct audio_hw_device *hw, struct audio_stream_out *s) {
    struct ga36_device *d=(struct ga36_device *)hw; pthread_mutex_lock(&d->lock);
    if (s && d->output==(struct ga36_output *)s) {
        standby(&s->common); pthread_mutex_destroy(&d->output->lock); free(d->output); d->output=NULL;
    }
    pthread_mutex_unlock(&d->lock);
}
static int init_check(const struct audio_hw_device *hw) {
    (void)hw; struct mixer *m=mixer_open(0); if(!m) return -ENODEV;
    bool valid=mixer_get_ctl_by_name(m,"Audio speaker out") && mixer_get_ctl_by_name(m,"Speaker Function") &&
        mixer_get_ctl_by_name(m,"speaker volume control");
    mixer_close(m); return valid ? 0 : -ENODEV;
}
static int unsupported_volume(struct audio_hw_device *d,float v) { (void)d;(void)v;return -ENOSYS; }
static int unsupported_get_volume(struct audio_hw_device *d,float *v) { (void)d;(void)v;return -ENOSYS; }
static int mode(struct audio_hw_device *d,audio_mode_t m) { (void)d;return m==AUDIO_MODE_IN_CALL ? -ENOSYS : 0; }
static int mic_mute(struct audio_hw_device *d,bool m) { (void)d;(void)m;return -ENOSYS; }
static int get_mic_mute(const struct audio_hw_device *d,bool *m) { (void)d;(void)m;return -ENOSYS; }
static int master_mute(struct audio_hw_device *d,bool m) { (void)d;(void)m;return -ENOSYS; }
static int get_master_mute(struct audio_hw_device *d,bool *m) { (void)d;(void)m;return -ENOSYS; }
static int device_parameters(struct audio_hw_device *d,const char *kv) { (void)d;(void)kv;return 0; }
static char *device_get_parameters(const struct audio_hw_device *d,const char *k) { (void)d;(void)k;return strdup(""); }
static size_t input_size(const struct audio_hw_device *d,const struct audio_config *c) { (void)d;(void)c;return 0; }
static int open_input(struct audio_hw_device *d,audio_io_handle_t h,audio_devices_t dev,
        struct audio_config *c,struct audio_stream_in **s,audio_input_flags_t f,const char *a,audio_source_t src) {
    (void)d;(void)h;(void)dev;(void)c;(void)f;(void)a;(void)src; if(s) *s=NULL; return -ENOSYS;
}
static void close_input(struct audio_hw_device *d,struct audio_stream_in *s) { (void)d;(void)s; }
static int device_dump(const struct audio_hw_device *d,int fd) {
    (void)d; dprintf(fd,"GA36 real ALSA card0 PCM0 speaker-only bring-up HAL; software mixing/volume\n"); return 0;
}
static int close_device(struct hw_device_t *hw) {
    struct ga36_device *d=(struct ga36_device *)hw; pthread_mutex_lock(&d->lock);
    if (d->output) { pthread_mutex_unlock(&d->lock); return -EBUSY; }
    pthread_mutex_unlock(&d->lock); pthread_mutex_destroy(&d->lock); free(d); return 0;
}
static int open_device(const struct hw_module_t *m,const char *name,struct hw_device_t **result) {
    if (!name || strcmp(name,AUDIO_HARDWARE_INTERFACE) || !result) return -EINVAL;
    *result=NULL; struct ga36_device *d=calloc(1,sizeof(*d)); if (!d) return -ENOMEM;
    int err=pthread_mutex_init(&d->lock,NULL); if(err) { free(d);return -err; }
    d->hw.common=(struct hw_device_t){ .tag=HARDWARE_DEVICE_TAG,.version=AUDIO_DEVICE_API_VERSION_2_0,
        .module=(struct hw_module_t *)(uintptr_t)m,.close=close_device };
    d->hw.init_check=init_check; d->hw.set_voice_volume=unsupported_volume;
    d->hw.set_master_volume=unsupported_volume; d->hw.get_master_volume=unsupported_get_volume;
    d->hw.set_mode=mode; d->hw.set_mic_mute=mic_mute; d->hw.get_mic_mute=get_mic_mute;
    d->hw.set_master_mute=master_mute; d->hw.get_master_mute=get_master_mute;
    d->hw.set_parameters=device_parameters; d->hw.get_parameters=device_get_parameters;
    d->hw.get_input_buffer_size=input_size; d->hw.open_input_stream=open_input; d->hw.close_input_stream=close_input;
    d->hw.open_output_stream=open_output; d->hw.close_output_stream=close_output; d->hw.dump=device_dump;
    *result=&d->hw.common; return 0;
}
static struct hw_module_methods_t methods={ .open=open_device };
struct audio_module HAL_MODULE_INFO_SYM={ .common={ .tag=HARDWARE_MODULE_TAG,
    .module_api_version=AUDIO_MODULE_API_VERSION_0_1,.hal_api_version=HARDWARE_HAL_API_VERSION,
    .id=AUDIO_HARDWARE_MODULE_ID,.name="GA36 A33 PCM speaker",.author="GA36 Android",
    .methods=&methods } };
