// unix_shell.c
// gcc unix-shell.c -o unix_shell
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXARGS 10
// MAIN
int main(int argc, const char *argv[])
{
    char input[BUFSIZ];
    char last_command[BUFSIZ];
    char *command[BUFSIZ];

    bool finished = false;
    bool complete = false;
    bool is_pipe = false;
    pid_t pid;
    
    //Set buffer to value
    memset(input, 0, BUFSIZ * sizeof(char));
    memset(last_command, 0, BUFSIZ * sizeof(char));

    while (!finished)
    {
        if (concur)
        {
            printf(" ");
            concur = false;
        }
        else
            printf("osh> ");
        fflush(stdout);

        // get user input
        if (*(fgets(input, BUFSIZ, stdin)) == '\n')
        {
            fprintf(stderr, "No command\n");
            exit(1);
        }

        int i;
        input[strlen(input) - 1] = '\0';
        // After reading user input, the steps are:
        // * (1) fork a child process using fork()
        // * (2) the child process will invoke execvp()
        // * (3) parent will invoke wait() unless command included &
        //  return 0;

        // input parsing
        if (strncmp(input, "!!", 2) == 0)
        {
            if (strlen(last_command) == 0)
            {
                fprintf(stderr, "No commands in history\n");
            }
            strcpy(input, last_command);
            printf("Last Command: %s\n", last_command);
        }
        else if (strncmp(input, "exit", 4) == 0)
        {
            finished = true;
            continue;
        }
        else
        {
            strcpy(last_command, input);
        }

        const char break_chars[] = " \t";
        char *p;
        i = 0;
        p = strtok(input, break_chars);

        while (p != NULL)
        {
            command[i++] = p;
            p = strtok(NULL, break_chars);
        }
        command[i] = NULL;

        // input for file input/output
        complete = (strncmp(command[i - 1], "&", 1) == 0);

        bool out = false;
        bool in = false;
        char *in_filename;
        char *out_filename;
        int k = 0;

        for (int j = 0; j < i; ++j)
        {
            if (strncmp(command[j], ">", 1) == 0)
            {
                out = true;
                out_filename = command[j + 1];
                if (k == 0)
                    k = j;
            }
            else if (strncmp(command[j], "<", 1) == 0)
            {
                in = true;
                in_filename = command[j + 1];
                if (k == 0)
                    k = j;
            }
        }

        if (k > 0)
        {
            command[k] = NULL;
            i = k;
        }

        // parsing for pipe
        char *command2[BUFSIZ];
        int n = 0;
        int fd_pipe[2];
        pid_t pid2;

        for (int j = 0; j < i; ++j)
        {
            if (strncmp(command[j], "|", 1) == 0)
            {
                is_pipe = true;
                n = j;
            }
        }

        if (is_pipe)
        {
            int m = 0;
            for (int j = n + 1; j < i; ++j, ++m)
            {
                command2[m] = command[j];
            }
            command[n] = NULL;

            if (pipe(fd_pipe) != 0)
            {
                fprintf(stderr, "Cannot create pipe\n");
                exit(1);
            }
        }

        // fork
        pid = fork();

        if (pid < 0)
        {
            fprintf(stderr, "Fork Failed");
            exit(1);
        }
        else if (pid == 0)
        {
            if (complete && (!out || !in))
                command[i - 1] = NULL;

            if (out)
            {
                int fd_out = creat(out_filename, 0644);
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            if (in)
            {
                int fd_in = open(in_filename, O_RDONLY);
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            // dup2 
            if (is_pipe)
            {
                dup2(fd_pipe[1], STDOUT_FILENO);
                close(fd_pipe[0]);
                close(fd_pipe[1]);
            }

            execvp(command[0], command);
        }
        else
        {
            if (is_pipe)
            {
                pid2 = fork();

                if (pid2 < 0)
                {
                    fprintf(stderr, "Fork Failed");
                    exit(1);
                }
                else if (pid2 == 0)
                {
                    dup2(fd_pipe[0], STDIN_FILENO);
                    close(fd_pipe[0]);
                    close(fd_pipe[1]);
                    execvp(command2[0], command2);
                }
                else
                {
                    wait(NULL);
                    is_pipe = false;
                }
            }
            else
            {
                if (!complete)
                    wait(NULL);
                else
                    signal(SIGCHLD, SIG_IGN);
            }
        }

        printf("\n");
    }

    printf("osh exited\n");

    return 0;
}
// OUTPUTS
// osh> ps -ael
// F S   UID   PID  PPID  C PRI  NI ADDR SZ WCHAN  TTY          TIME CMD
// 4 S     0     1     0  0  80   0 -  9471 -      ?        00:00:04 systemd
// 1 S     0     2     0  0  80   0 -     0 -      ?        00:00:00 kthreadd
// 1 S     0     3     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/0
// 1 S     0     5     2  0  60 -20 -     0 -      ?        00:00:00 kworker/0:0H
// 1 S     0     7     2  0  80   0 -     0 -      ?        00:00:01 rcu_sched
// 1 S     0     8     2  0  80   0 -     0 -      ?        00:00:00 rcu_bh
// 1 S     0     9     2  0 -40   - -     0 -      ?        00:00:00 migration/0
// 5 S     0    10     2  0 -40   - -     0 -      ?        00:00:00 watchdog/0
// 5 S     0    11     2  0 -40   - -     0 -      ?        00:00:00 watchdog/1
// 1 S     0    12     2  0 -40   - -     0 -      ?        00:00:00 migration/1
// 1 S     0    13     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/1
// 1 S     0    15     2  0  60 -20 -     0 -      ?        00:00:00 kworker/1:0H
// 5 S     0    16     2  0 -40   - -     0 -      ?        00:00:00 watchdog/2
// 1 S     0    17     2  0 -40   - -     0 -      ?        00:00:00 migration/2
// 1 S     0    18     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/2
// 1 S     0    20     2  0  60 -20 -     0 -      ?        00:00:00 kworker/2:0H
// 5 S     0    21     2  0 -40   - -     0 -      ?        00:00:00 watchdog/3
// 1 S     0    22     2  0 -40   - -     0 -      ?        00:00:00 migration/3
// 1 S     0    23     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/3
// 1 S     0    25     2  0  60 -20 -     0 -      ?        00:00:00 kworker/3:0H
// 5 S     0    26     2  0  80   0 -     0 -      ?        00:00:00 kdevtmpfs
// 1 S     0    27     2  0  60 -20 -     0 -      ?        00:00:00 netns
// 1 S     0    28     2  0  60 -20 -     0 -      ?        00:00:00 perf
// 1 S     0    29     2  0  80   0 -     0 -      ?        00:00:00 khungtaskd
// 1 S     0    30     2  0  60 -20 -     0 -      ?        00:00:00 writeback
// 1 S     0    31     2  0  85   5 -     0 -      ?        00:00:00 ksmd
// 1 S     0    32     2  0  99  19 -     0 -      ?        00:00:00 khugepaged
// 1 S     0    33     2  0  60 -20 -     0 -      ?        00:00:00 crypto
// 1 S     0    34     2  0  60 -20 -     0 -      ?        00:00:00 kintegrityd
// 1 S     0    35     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    36     2  0  60 -20 -     0 -      ?        00:00:00 kblockd
// 1 S     0    37     2  0  60 -20 -     0 -      ?        00:00:00 ata_sff
// 1 S     0    38     2  0  60 -20 -     0 -      ?        00:00:00 md
// 1 S     0    39     2  0  60 -20 -     0 -      ?        00:00:00 devfreq_wq
// 1 S     0    41     2  0  80   0 -     0 -      ?        00:00:00 kworker/1:1
// 1 S     0    43     2  0  80   0 -     0 -      ?        00:00:00 kworker/3:1
// 1 S     0    44     2  0  80   0 -     0 -      ?        00:00:00 kworker/2:1
// 1 S     0    46     2  0  80   0 -     0 -      ?        00:00:00 kswapd0
// 1 S     0    47     2  0  60 -20 -     0 -      ?        00:00:00 vmstat
// 1 S     0    48     2  0  80   0 -     0 -      ?        00:00:00 fsnotify_mark
// 1 S     0    49     2  0  80   0 -     0 -      ?        00:00:00 ecryptfs-kthrea
// 1 S     0    65     2  0  60 -20 -     0 -      ?        00:00:00 kthrotld
// 1 S     0    66     2  0  60 -20 -     0 -      ?        00:00:00 acpi_thermal_pm
// 1 S     0    67     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    68     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    69     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    70     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    71     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    72     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    73     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    74     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    75     2  0  80   0 -     0 -      ?        00:00:00 scsi_eh_0
// 1 S     0    76     2  0  60 -20 -     0 -      ?        00:00:00 scsi_tmf_0
// 1 S     0    77     2  0  80   0 -     0 -      ?        00:00:00 scsi_eh_1
// 1 S     0    78     2  0  60 -20 -     0 -      ?        00:00:00 scsi_tmf_1
// 1 S     0    84     2  0  60 -20 -     0 -      ?        00:00:00 ipv6_addrconf
// 1 S     0    97     2  0  60 -20 -     0 -      ?        00:00:00 deferwq
// 1 S     0    98     2  0  60 -20 -     0 -      ?        00:00:00 charger_manager
// 1 S     0    99     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0   143     2  0  80   0 -     0 -      ?        00:00:00 scsi_eh_2
// 1 S     0   144     2  0  60 -20 -     0 -      ?        00:00:00 scsi_tmf_2
// 1 S     0   154     2  0  60 -20 -     0 -      ?        00:00:00 kpsmoused
// 1 S     0   183     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0   188     2  0  60 -20 -     0 -      ?        00:00:00 kworker/3:1H
// 1 S     0   253     2  0  60 -20 -     0 -      ?        00:00:00 kworker/0:1H
// 1 S     0   260     2  0  60 -20 -     0 -      ?        00:00:00 raid5wq
// 1 S     0   293     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0   320     2  0  80   0 -     0 -      ?        00:00:00 jbd2/sda1-8
// 1 S     0   321     2  0  60 -20 -     0 -      ?        00:00:00 ext4-rsv-conver
// 1 S     0   366     2  0  80   0 -     0 -      ?        00:00:00 kworker/3:2
// 1 S     0   373     2  0  60 -20 -     0 -      ?        00:00:00 kworker/1:1H
// 1 S     0   383     2  0  80   0 -     0 -      ?        00:00:00 kworker/2:2
// 4 S     0   385     1  0  80   0 -  8446 -      ?        00:00:00 systemd-journal
// 1 S     0   395     2  0  60 -20 -     0 -      ?        00:00:00 iscsi_eh
// 1 S     0   400     2  0  80   0 -     0 -      ?        00:00:00 kauditd
// 1 S     0   408     2  0  60 -20 -     0 -      ?        00:00:00 ib_addr
// 1 S     0   412     2  0  60 -20 -     0 -      ?        00:00:00 ib_mcast
// 1 S     0   413     2  0  60 -20 -     0 -      ?        00:00:00 ib_nl_sa_wq
// 1 S     0   415     2  0  60 -20 -     0 -      ?        00:00:00 ib_cm
// 1 S     0   416     2  0  60 -20 -     0 -      ?        00:00:00 iw_cm_wq
// 1 S     0   417     2  0  60 -20 -     0 -      ?        00:00:00 rdma_cm
// 4 S     0   418     1  0  80   0 - 23692 -      ?        00:00:00 lvmetad
// 4 S     0   441     1  0  80   0 - 11182 -      ?        00:00:00 systemd-udevd
// 1 S     0   505     2  0  60 -20 -     0 -      ?        00:00:00 iprt-VBoxWQueue
// 1 S     0   515     2  0  60 -20 -     0 -      ?        00:00:00 kworker/2:1H
// 1 S     0   623     2  0  80   0 -     0 -      ?        00:00:00 kworker/0:4
// 1 S     0   625     2  0  80   0 -     0 -      ?        00:00:00 kworker/0:5
// 1 S     0   646     2  0  60 -20 -     0 -      ?        00:00:00 ttm_swap
// 4 S   100   649     1  0  80   0 - 25080 -      ?        00:00:00 systemd-timesyn
// 4 S   104   821     1  0  80   0 - 64097 -      ?        00:00:00 rsyslogd
// 4 S     1   826     1  0  80   0 -  6510 -      ?        00:00:00 atd
// 4 S   107   828     1  0  80   0 - 10724 -      ?        00:00:00 dbus-daemon
// 1 S     0   863     1  0  80   0 -  4029 -      ?        00:00:00 dhclient
// 4 S     0   895     1  0  80   0 - 23841 -      ?        00:00:00 lxcfs
// 0 S     0   898     1  0  80   0 -  1098 -      ?        00:00:00 acpid
// 4 S     0   901     1  0  80   0 -  7168 -      ?        00:00:00 systemd-logind
// 4 S     0   906     1  0  80   0 -  7251 -      ?        00:00:00 cron
// 4 S     0   912     1  0  80   0 - 68968 -      ?        00:00:00 accounts-daemon
// 4 S     0   914     1  0  80   0 - 62110 -      ?        00:00:00 snapd
// 1 S     0   974     1  0  80   0 -  3342 -      ?        00:00:00 mdadm
// 4 S     0   995     1  0  80   0 - 16376 -      ?        00:00:00 sshd
// 1 S     0  1019     1  0  80   0 -  1304 -      ?        00:00:00 iscsid
// 4 S     0  1020     1  0  80   0 - 69277 -      ?        00:00:00 polkitd
// 5 S     0  1021     1  0  70 -10 -  1429 -      ?        00:00:00 iscsid
// 5 S     0  1091     1  0  80   0 -  4867 -      ?        00:00:00 irqbalance
// 4 S     0  1103     1  0  80   0 - 16980 -      tty1     00:00:00 login
// 4 S     0  1449   995  0  80   0 - 23730 -      ?        00:00:00 sshd
// 4 S  1000  1502     1  0  80   0 - 11318 ep_pol ?        00:00:00 systemd
// 5 S  1000  1506  1502  0  80   0 - 15857 -      ?        00:00:00 (sd-pam)
// 4 S  1000  1509  1103  0  80   0 -  5648 wait_w tty1     00:00:00 bash
// 5 S  1000  1550  1449  0  80   0 - 23767 -      ?        00:00:04 sshd
// 0 S  1000  1551  1550  0  80   0 -  3170 wait   ?        00:00:00 bash
// 0 S  1000  1596  1551  0  80   0 -  1125 wait   ?        00:00:00 sh
// 0 R  1000  1603  1596  0  80   0 - 265819 -     ?        00:00:12 node
// 0 S  1000  1704  1603  0  80   0 - 217398 ep_pol ?       00:00:03 node
// 0 S  1000  1717  1603  3  80   0 - 341172 ep_pol ?       00:01:48 node
// 0 S  1000  1840     1  0  80   0 -  1125 wait   ?        00:00:00 sh
// 0 S  1000  1841  1840  0  80   0 - 820645 futex_ ?       00:00:02 vsls-agent
// 0 S  1000  1900  1717  0  80   0 - 379401 unix_s ?       00:00:22 cpptools
// 0 S  1000  1984  1603  0  80   0 -  5629 wait_w pts/0    00:00:00 bash
// 0 S  1000  2491  1717  0  80   0 - 154663 ep_pol ?       00:00:01 node
// 1 S     0  3922     2  0  80   0 -     0 -      ?        00:00:00 kworker/1:0
// 0 T  1000  6184  1984  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 0 T  1000  7186  6184  0  80   0 -  2413 signal pts/0    00:00:00 less
// 1 S     0  7235     2  0  80   0 -     0 -      ?        00:00:00 kworker/u8:0
// 1 T  1000  7566  6184  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7567  7566  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7568  7567  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7569  7568  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7570  7569  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7571  7570  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7572  7571  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7573  7572  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7574  7573  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7575  7574  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7576  7575  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7577  7576  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7578  7577  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7579  7578  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7580  7579  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7581  7580  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7582  7581  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7583  7582  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7584  7583  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7585  7584  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7586  7585  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7587  7586  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7588  7587  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7589  7588  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7590  7589  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 T  1000  7591  7590  0  80   0 -  1094 signal pts/0    00:00:00 shell
// 1 S     0  8109     2  0  80   0 -     0 -      ?        00:00:00 kworker/u8:2
// 0 S  1000  8181  1900  0  80   0 - 1241904 futex_ ?      00:00:00 cpptools-srv
// 0 S  1000  9184  1603  0  80   0 -  5629 wait   pts/1    00:00:00 bash
// 1 S     0  9557     2  0  80   0 -     0 -      ?        00:00:00 kworker/u8:1
// 0 S  1000  9584  1900  0  80   0 - 1241904 futex_ ?      00:00:00 cpptools-srv
// 0 S  1000  9624  1551  0  80   0 -  1821 hrtime ?        00:00:00 sleep
// 0 S  1000  9774  9184  0  80   0 -  1095 wait   pts/1    00:00:00 unix_shell
// 0 R  1000  9796  9774  0  80   0 -  7228 -      pts/1    00:00:00 ps

// osh> ls > out.txt

// osh> sort < out.txt
// a.out
// bin
// ch10
// ch13
// ch19
// ch2
// ch3
// ch4
// ch5
// ch7
// ch8
// dining_philo.c
// include
// NoVisual
// out.txt
// Philosopher.cpp
// Philosopher.h
// Philosopher-main.cpp
// protect_critical_region
// protect_critical_region.c
// ps
// shell
// test_shell.c
// ThreadMentor-Linux64.tar.gz
// unix_shell
// unix_shell.c
// Visual

// osh> cat out.txt
// a.out
// bin
// ch10
// ch13
// ch19
// ch2
// ch3
// ch4
// ch5
// ch7
// ch8
// dining_philo.c
// include
// NoVisual
// out.txt
// Philosopher.cpp
// Philosopher.h
// Philosopher-main.cpp
// protect_critical_region
// protect_critical_region.c
// ps
// shell
// test_shell.c
// ThreadMentor-Linux64.tar.gz
// unix_shell
// unix_shell.c
// Visual

// osh> !!
// Last Command: cat out.txt
// a.out
// bin
// ch10
// ch13
// ch19
// ch2
// ch3
// ch4
// ch5
// ch7
// ch8
// dining_philo.c
// include
// NoVisual
// out.txt
// Philosopher.cpp
// Philosopher.h
// Philosopher-main.cpp
// protect_critical_region
// protect_critical_region.c
// ps
// shell
// test_shell.c
// ThreadMentor-Linux64.tar.gz
// unix_shell
// unix_shell.c
// Visual


// osh> ls -l
// total 2308
// -rwxrwxr-x 1 osc osc   13736 Mar 24 18:00 a.out
// drwx------ 2 osc osc    4096 Mar 20 14:50 bin
// drwxrwxr-x 2 osc osc    4096 Jan  3  2018 ch10
// drwxrwxr-x 2 osc osc    4096 Jun 20  2018 ch13
// drwxrwxr-x 2 osc osc    4096 Jun 20  2018 ch19
// drwxrwxr-x 5 osc osc    4096 Mar 21 16:54 ch2
// drwxrwxr-x 2 osc osc    4096 Feb 13 12:52 ch3
// drwxrwxr-x 2 osc osc    4096 Feb 27 11:22 ch4
// drwxrwxr-x 3 osc osc    4096 Jun 19  2018 ch5
// drwxrwxr-x 3 osc osc    4096 Aug 11  2018 ch7
// drwxrwxr-x 2 osc osc    4096 Jun 20  2018 ch8
// -rw-rw-r-- 1 osc osc    3726 Mar 23 21:18 dining_philo.c
// drwx------ 2 osc osc    4096 Mar 20 14:50 include
// drwx------ 2 osc osc    4096 Mar 20 14:50 NoVisual
// -rw-r--r-- 1 osc osc     271 Mar 24 18:43 out.txt
// -rw-rw-r-- 1 osc osc    2599 Sep 28  2005 Philosopher.cpp
// -rw-rw-r-- 1 osc osc     514 Sep 28  2005 Philosopher.h
// -rw-rw-r-- 1 osc osc    1101 Sep 28  2005 Philosopher-main.cpp
// -rwxrwxr-x 1 osc osc    9264 Mar 14 13:20 protect_critical_region
// -rw-rw-r-- 1 osc osc    5708 Mar 14 13:50 protect_critical_region.c
// -rw-rw-r-- 1 osc osc    6040 Mar 24 17:53 ps
// -rwxrwxr-x 1 osc osc   13920 Mar 24 18:39 shell
// -rw-rw-r-- 1 osc osc   25666 Mar 24 18:31 test_shell.c
// -rw-rw-r-- 1 osc osc 2156026 May  2  2014 ThreadMentor-Linux64.tar.gz
// -rwxrwxr-x 1 osc osc   13920 Mar 24 18:41 unix_shell
// -rw-rw-r-- 1 osc osc   20837 Mar 24 18:45 unix_shell.c
// drwx------ 2 osc osc    4096 Mar 20 14:50 Visual

// osh> total 2308
// -rwxrwxr-x 1 osc osc   13736 Mar 24 18:00 a.out
// drwx------ 2 osc osc    4096 Mar 20 14:50 bin
// drwxrwxr-x 2 osc osc    4096 Jan  3  2018 ch10
// drwxrwxr-x 2 osc osc    4096 Jun 20  2018 ch13
// drwxrwxr-x 2 osc osc    4096 Jun 20  2018 ch19
// drwxrwxr-x 5 osc osc    4096 Mar 21 16:54 ch2
// drwxrwxr-x 2 osc osc    4096 Feb 13 12:52 ch3
// drwxrwxr-x 2 osc osc    4096 Feb 27 11:22 ch4
// drwxrwxr-x 3 osc osc    4096 Jun 19  2018 ch5
// drwxrwxr-x 3 osc osc    4096 Aug 11  2018 ch7
// drwxrwxr-x 2 osc osc    4096 Jun 20  2018 ch8
// -rw-rw-r-- 1 osc osc    3726 Mar 23 21:18 dining_philo.c
// drwx------ 2 osc osc    4096 Mar 20 14:50 include
// drwx------ 2 osc osc    4096 Mar 20 14:50 NoVisual
// -rw-r--r-- 1 osc osc     271 Mar 24 18:43 out.txt
// -rw-rw-r-- 1 osc osc    2599 Sep 28  2005 Philosopher.cpp
// -rw-rw-r-- 1 osc osc     514 Sep 28  2005 Philosopher.h
// -rw-rw-r-- 1 osc osc    1101 Sep 28  2005 Philosopher-main.cpp
// -rwxrwxr-x 1 osc osc    9264 Mar 14 13:20 protect_critical_region
// -rw-rw-r-- 1 osc osc    5708 Mar 14 13:50 protect_critical_region.c
// -rw-rw-r-- 1 osc osc    6040 Mar 24 17:53 ps
// -rwxrwxr-x 1 osc osc   13920 Mar 24 18:39 shell
// -rw-rw-r-- 1 osc osc   25666 Mar 24 18:31 test_shell.c
