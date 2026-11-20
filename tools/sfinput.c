/* sfinput: inject multi-touch input on a Sailfish OS device for UI tests.
 *
 * Creates a virtual touchscreen through /dev/uinput (needs root), runs the
 * given commands, then removes the device. Coordinates are portrait screen
 * pixels (Jolla C2: 720 x 1600).
 *
 *   sfinput [-w W] [-h H] CMD...
 *     tap X Y                  quick tap
 *     drag X1 Y1 X2 Y2 MS      press, move, release
 *     down SLOT X Y            put finger SLOT (0-9) down
 *     move SLOT X Y            move finger SLOT
 *     up SLOT                  lift finger SLOT
 *     sleep MS
 *     key CODE                 press and release a key (linux input code)
 *
 * Example, hold A and B together for half a second:
 *     sfinput "down 0 560 1050" "down 1 450 1150" "sleep 500" "up 0" "up 1"
 */
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static int fd;
static int width = 720, height = 1600;
static int active[10];
static int nextId = 100;

static void msleep(int ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void emit(int type, int code, int value)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
        perror("write");
}

static void syn(void)
{
    int any = 0;
    for (int i = 0; i < 10; ++i)
        any |= active[i];
    emit(EV_KEY, BTN_TOUCH, any);
    emit(EV_SYN, SYN_REPORT, 0);
}

static void finger(int slot, int x, int y, int down)
{
    emit(EV_ABS, ABS_MT_SLOT, slot);
    if (down && !active[slot]) {
        emit(EV_ABS, ABS_MT_TRACKING_ID, nextId++);
        active[slot] = 1;
    }
    emit(EV_ABS, ABS_MT_POSITION_X, x);
    emit(EV_ABS, ABS_MT_POSITION_Y, y);
    emit(EV_ABS, ABS_MT_TOUCH_MAJOR, 10);
    emit(EV_ABS, ABS_X, x);
    emit(EV_ABS, ABS_Y, y);
}

static void lift(int slot)
{
    emit(EV_ABS, ABS_MT_SLOT, slot);
    emit(EV_ABS, ABS_MT_TRACKING_ID, -1);
    active[slot] = 0;
}

static struct uinput_user_dev dev;

static void absSetup(int code, int max)
{
    dev.absmin[code] = 0;
    dev.absmax[code] = max;
}

int main(int argc, char **argv)
{
    int first = 1;
    while (first + 1 < argc && argv[first][0] == '-') {
        if (!strcmp(argv[first], "-w"))
            width = atoi(argv[first + 1]);
        else if (!strcmp(argv[first], "-h"))
            height = atoi(argv[first + 1]);
        first += 2;
    }

    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("/dev/uinput");
        return 1;
    }
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    for (int k = 1; k < 249; ++k)
        ioctl(fd, UI_SET_KEYBIT, k);
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    absSetup(ABS_X, width - 1);
    absSetup(ABS_Y, height - 1);
    absSetup(ABS_MT_SLOT, 9);
    absSetup(ABS_MT_TRACKING_ID, 65535);
    absSetup(ABS_MT_POSITION_X, width - 1);
    absSetup(ABS_MT_POSITION_Y, height - 1);
    absSetup(ABS_MT_TOUCH_MAJOR, 255);

    // The legacy setup call: the SDK's kernel headers predate UI_DEV_SETUP.
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = 0x1234;
    dev.id.product = 0x5678;
    strcpy(dev.name, "sfinput virtual touchscreen");
    if (write(fd, &dev, sizeof(dev)) != sizeof(dev) || ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("uinput create");
        return 1;
    }
    // Give the compositor time to notice the new device.
    msleep(1500);

    for (int i = first; i < argc; ++i) {
        char cmd[16];
        int a = 0, b = 0, c = 0, d = 0, e = 0;
        int n = sscanf(argv[i], "%15s %d %d %d %d %d", cmd, &a, &b, &c, &d, &e);
        if (n < 1)
            continue;
        if (!strcmp(cmd, "tap") && n >= 3) {
            finger(0, a, b, 1);
            syn();
            msleep(60);
            lift(0);
            syn();
            msleep(60);
        } else if (!strcmp(cmd, "drag") && n >= 5) {
            int ms = n >= 6 ? e : 300;
            int steps = ms / 12 > 2 ? ms / 12 : 2;
            finger(0, a, b, 1);
            syn();
            for (int s = 1; s <= steps; ++s) {
                msleep(ms / steps);
                finger(0, a + (c - a) * s / steps, b + (d - b) * s / steps, 1);
                syn();
            }
            msleep(30);
            lift(0);
            syn();
        } else if (!strcmp(cmd, "down") && n >= 4) {
            finger(a, b, c, 1);
            syn();
        } else if (!strcmp(cmd, "move") && n >= 4) {
            finger(a, b, c, 1);
            syn();
        } else if (!strcmp(cmd, "up") && n >= 2) {
            lift(a);
            syn();
        } else if (!strcmp(cmd, "sleep") && n >= 2) {
            msleep(a);
        } else if (!strcmp(cmd, "key") && n >= 2) {
            emit(EV_KEY, a, 1);
            emit(EV_SYN, SYN_REPORT, 0);
            msleep(50);
            emit(EV_KEY, a, 0);
            emit(EV_SYN, SYN_REPORT, 0);
        } else {
            fprintf(stderr, "bad command: %s\n", argv[i]);
        }
    }

    for (int s = 0; s < 10; ++s)
        if (active[s])
            lift(s);
    syn();
    msleep(200);
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
