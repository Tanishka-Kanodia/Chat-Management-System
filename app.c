#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_LINES_INPUT 34

struct msgbuf1
{
    long mtype;
    char mtext[MAX_LINE_LENGTH];
};

int count_active_process() {
    int count = 0;
    FILE *fp=popen("pgrep -c groups.out","r");
    if(fp==NULL)
    {
        perror("popen lite! Cant check status of process.");
        exit(1);
    }
    fscanf(fp,"%d",&count);
    pclose(fp);
    return count;
}

int main(int argc, char*argv[])
{
    int x;
    int n;
    char filepath[100];
    sscanf(argv[1],"%d",&x);
    char buffer[MAX_LINES_INPUT][MAX_LINE_LENGTH];
    snprintf(filepath,sizeof(filepath),"testcase_%d/%s",x,"input.txt");
    FILE*file=fopen(filepath,"r");
    if(file==NULL)
    {
        perror("Error, file not found!\n");
        return 1;
    }
    int count=0;
    int row=0;
    while(fgets(buffer[row],sizeof(buffer),file))
    {
        buffer[row][strcspn(buffer[row],"\n")]=0;
        if(count==0)
        {
            n=atoi(buffer[row]);
            count++;
        }
        row++;
    }
    fclose(file);
    int key=atoi(buffer[2]);
    /////////////////////////////////////////////////////////////////////////////
    for(int i=0;i<n;i++)
    {
        pid_t pid=fork();
        

        if(pid<0)
        {
            perror("Fork lite! No new process created.\n");
            exit(1);
        }
        else if(pid==0)
        {
            //char id_str[10];
            execl("./groups.out", "groups.out",buffer[2],NULL);
            perror("execl lite! Process definition did not change.\n");
            exit(1);
        }
        else
        {
            struct msgbuf1 message;
            message.mtype=1;
            /*char num_str[10];
            int index=0;
            for(int j=13;j<16 && isdigit(buffer[i+5][j]);j++) {
                num_str[index++] = buffer[i][j];
            }
            strcpy(message.mtext,num_str);*/
            //strcpy(message.mtext,buffer[0]);
            int msgid = msgget(key, 0666 | IPC_CREAT);
            //msgsnd(msgid,&message,sizeof(message.mtext),0);
            memset(message.mtext, 0, sizeof(message.mtext));  // Clears old content
            strcpy(message.mtext,buffer[1]);
            msgsnd(msgid,&message,sizeof(message.mtext),0);
            memset(message.mtext, 0, sizeof(message.mtext));  // Clears old content
            strcpy(message.mtext,buffer[3]);
            msgsnd(msgid,&message,sizeof(message.mtext),0);
            memset(message.mtext, 0, sizeof(message.mtext));  // Clears old content
            strcpy(message.mtext,buffer[5+i]);
            msgsnd(msgid,&message,sizeof(message.mtext),0);
            memset(message.mtext, 0, sizeof(message.mtext));  // Clears old content
            /*char test[1];
            test[0]=x+'0';
            strcpy(message.mtext,test);
            msgsnd(msgid,&message,sizeof(message.mtext),0);*/
            //sending testcase number
       	 
	char str[2]; // 1 digit + null terminator

	str[0] = x + '0'; // Convert to character
	str[1] = '\0'; // Null-terminate the string
        	strcpy(message.mtext, str);	// Integer to send
	if (msgsnd(msgid, &message, sizeof(message.mtext), 0) == -1) {
    	perror("msgsnd failed");
    	exit(1);
	}

	printf("Sent integer: %s\n", message.mtext);

            sleep(1);
        }
    }

    while(count_active_process()>0)
    {
        struct msgbuf1 message1;
        message1.mtype=1;
        int msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid == -1)
        {
            perror("Error in msgget");
            return 1;
        }
        if(msgrcv(msgid, &message1, sizeof(message1.mtext), 1, 0) != -1)
        {
            printf("All users terminated. Exiting group process %s\n",message1.mtext);
        }
        msgctl(msgid, IPC_RMID, NULL);
    }
    wait(NULL);
    return 0;
}