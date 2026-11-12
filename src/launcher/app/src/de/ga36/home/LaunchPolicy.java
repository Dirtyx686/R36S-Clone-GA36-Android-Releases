package de.ga36.home;
final class LaunchPolicy {
 static int flags(boolean internal,int original){final int newTask=0x10000000,reset=0x00200000,noAnimation=0x00010000;return internal?(original&~(newTask|reset))|noAnimation:original|newTask|reset|noAnimation;}
}
