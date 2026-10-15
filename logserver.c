#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdarg.h>

#define FIFO_PATH       "/tmp/logserver.fifo"
#define LOG_FILE        "/tmp/logserver.log"
#define ALARM_PERIOD    5
#define BUF_SIZE        4096

static volatile sig_atomic_t g_stop_sigterm = 0;
static volatile sig_atomic_t g_stop_sigint  = 0;
static volatile sig_atomic_t g_alarm_fired  = 0;
static volatile sig_atomic_t g_alarm_count  = 0;
static volatile sig_atomic_t g_print_stats  = 0;
static volatile sig_atomic_t g_daemonize    = 0;

static FILE *g_log = NULL;

static void logmsg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

static void on_sigterm(int) { g_stop_sigterm = 1; }
static void on_sigint (int) { g_stop_sigint  = 1; }
static void on_sigalrm(int) { g_alarm_fired  = 1; g_alarm_count++; }
static void on_sigquit(int) {}
static void on_sigusr1(int) { g_print_stats = 1; }
static void on_sighup (int) { g_daemonize = 1; }

static void install_handler(int signum, void (*handler)(int))
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signum, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction(%d): %s\n", signum, strerror(errno));
        exit(5);
    }
}

static void prepare_fifo(const char *path)
{
    if (mkfifo(path, 0600) == -1) {
        if (errno != EEXIST) {
            fprintf(stderr, "mkfifo(%s): %s\n", path, strerror(errno));
            exit(1);
        }
        
        struct stat st;
        if (stat(path, &st) == -1) {
            fprintf(stderr, "stat(%s): %s\n", path, strerror(errno));
            exit(1);
        }
        if (!S_ISFIFO(st.st_mode)) {
            fprintf(stderr, "Error: %s already exists (not FIFO)\n", path);
            exit(1);
        }
    }
}

int g_msg_count = 0;
unsigned long long g_bytes_total = 0;

static void print_stats()
{
    logmsg("========= Statistics =========");
    logmsg("  messages         : %d", g_msg_count);
    logmsg("  total bytes read : %llu", g_bytes_total);
    logmsg("  alarm firings    : %d", (int)g_alarm_count);
    logmsg("==============================");
}

int g_is_daemon = 0;

void daemonize() {
    if (g_is_daemon) {
        return;
    }

    g_is_daemon = 1;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(3);
    }
    if (pid > 0) {
        exit(0);
    }
     
    if (setsid() == -1) {
        perror("setsid");
        exit(3);
    }

    umask(0);
    if (chdir("/") == -1) {
        perror("chdir");
        exit(4);
    }
     
    int fd_null = open("/dev/null", O_RDWR);
    if (fd_null != -1) {
        dup2(fd_null, STDIN_FILENO);
        dup2(fd_null, STDOUT_FILENO);
        dup2(fd_null, STDERR_FILENO);
        if (fd_null > 2) {
            close(fd_null);
        }
    }

    g_log = fopen(LOG_FILE, "a");
    if (!g_log) {
        exit(4);
    }

    logmsg("[Became daemon from foreground process]");
    print_stats();
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = 1;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return 2;
        }
    }

    if (daemon_mode) {
        g_is_daemon = 1;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 3;
        }
        if (pid > 0) {
            return 0;
        }
        
        if (setsid() == -1) {
            perror("setsid");
            return 3;
        }

        umask(0);
        if (chdir("/") == -1) {
            perror("chdir");
            return 4;
        }
        
        int fd_null = open("/dev/null", O_RDWR);
        if (fd_null != -1) {
            dup2(fd_null, STDIN_FILENO);
            dup2(fd_null, STDOUT_FILENO);
            dup2(fd_null, STDERR_FILENO);
            if (fd_null > 2) {
                close(fd_null);
            }
        }

        g_log = fopen(LOG_FILE, "a");
        if (!g_log) {
            return 4;
        }
    } else {
        g_log = stdout;
    }

    prepare_fifo(FIFO_PATH);
    logmsg("Log-server is starting... FIFO: %s", FIFO_PATH);

    install_handler(SIGTERM, on_sigterm);
    install_handler(SIGINT,  on_sigint);
    install_handler(SIGALRM, on_sigalrm);
    install_handler(SIGQUIT, on_sigquit);
    install_handler(SIGUSR1, on_sigusr1);
    install_handler(SIGHUP, on_sighup);

    alarm(ALARM_PERIOD);
    
    int is_error_exit = 0;
    int working = 1;
    while (!g_stop_sigterm && !g_stop_sigint && working) {
        int fd;
        for (;;) {
            fd = open(FIFO_PATH, O_RDONLY);
            if (fd >= 0) {
                break;
            }

            if (errno == EINTR) {
                if (g_daemonize) {
                    g_daemonize = 0;
                    daemonize();
                    continue;
                }
                if (g_print_stats) {
                    g_print_stats = 0;
                    print_stats();
                    continue;
                }
                if (g_alarm_fired) {
                    g_alarm_fired = 0;
                    logmsg("Log-server is waiting for data...");
                    alarm(ALARM_PERIOD);
                    continue;
                }
                if (g_stop_sigterm) {
                    logmsg("Stopping work: SIGTERM");
                    working = 0;
                    break;
                }
                if (g_stop_sigint) {
                    logmsg("Stopping work: SIGINT");
                    working = 0;
                    break;
                }
                continue;
            }
            
            logmsg("open(%s): %s", FIFO_PATH, strerror(errno));
            is_error_exit = 1;
            working = 0;
            break;
        }

        if (!working) {
            break;
        }

        for (;;) {
            char buf[BUF_SIZE + 1];
            ssize_t n = read(fd, buf, BUF_SIZE);

            if (n > 0) {
                buf[n] = '\0';
                fputs(buf, g_log);
                fflush(g_log);
                g_bytes_total += (unsigned long long)n;
                
                if (g_stop_sigterm) {
                    logmsg("Stopping work: SIGTERM");
                    close(fd);
                    working = 0;
                    break;
                }
                continue;
            }

            if (n == 0) {
                break;
            }

            if (errno == EINTR) {
                if (g_daemonize) {
                    g_daemonize = 0;
                    daemonize();
                    continue;
                }
                if (g_print_stats) {
                    g_print_stats = 0;
                    print_stats();
                    continue;
                }
                if (g_alarm_fired) {
                    g_alarm_fired = 0;
                    logmsg("Log-server is waiting for data...");
                    alarm(ALARM_PERIOD);
                    continue;
                }

                if (g_stop_sigterm) {
                    logmsg("Stopping work: SIGTERM");
                    close(fd);
                    working = 0;
                    break;
                }

                if (g_stop_sigint) {
                    logmsg("Got SIGINT. Reading last data...");
                    
                    while (1) {
                        n = read(fd, buf, BUF_SIZE);

                        if (n == 0) {
                            break;
                        }
                        if (n < 0) {
                            if (errno != EINTR) {
                                logmsg("read: %s", strerror(errno));
                                close(fd);
                                is_error_exit = 1;
                                working = 0;
                                break;
                            }

                            if (g_daemonize) {
                                g_daemonize = 0;
                                daemonize();
                                continue;
                            }

                            if (g_print_stats) {
                                g_print_stats = 0;
                                print_stats();
                                continue;
                            }

                            if (g_alarm_fired) {
                                g_alarm_fired = 0;
                                continue;
                            }
                        
                            if (g_stop_sigterm) {
                                logmsg("Stopping work: SIGTERM");
                                close(fd);
                                working = 0;
                                break;
                            }
                            continue;
                        }

                        buf[n] = '\0';
                        fputs(buf, g_log);
                        fflush(g_log);
                        g_bytes_total += (unsigned long long)n;
                    }

                    if (!working) {
                        break;
                    }
                    
                    fputc('\n', g_log);
                    fflush(g_log);
                    close(fd);
                    logmsg("Stopping work: SIGINT");
                    working = 0;
                    break;
                }
                
                continue;
            }

            logmsg("read: %s", strerror(errno));
            close(fd);
            is_error_exit = 1;
            working = 0;
            break;
        }

        if (working) {
            g_msg_count++;
            close(fd);
        }
    }

    alarm(0);

    print_stats();

    if (unlink(FIFO_PATH) == -1 && errno != ENOENT) {
        logmsg("unlink(%s): %s", FIFO_PATH, strerror(errno));
        is_error_exit = 1;
    }
    if (g_log != stdout) {
        fclose(g_log);
    }

    if (is_error_exit) {
        return 1;
    }
    return 0;
}