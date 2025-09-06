#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<ctype.h>
#include<stdbool.h>
#include<sys/time.h>

#define MAX_LINE_LENGTH 256
#define MAX_LINES_INPUT 34
#define MAX_FILTERED_WORDS 50    // Maximum number of filtered words
#define MAX_WORD_LENGTH    20     // Maximum length of each filtered word
#define MAX_INPUT_LENGTH   256   // Maximum length of user input
#define TIMEOUT_SECONDS 5

void toLowerCase(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower(str[i]);
    }
}

long getCurrentTime(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec;
}
//////////////////////////////////////////////////////////////////////////////
int find_row(int group_num, int user_num, int arr[][3]) {
    for (int i = 0; i < 1000; i++) {
        if (arr[i][0] == group_num && arr[i][1] == user_num) {
            return i; // Return the row index if found
        }
    }
    return -1; // Return -1 if not found
}

bool add_or_update_data(int group_num, int user_num, int violations, int arr[][3]) {
    int row_index = find_row(group_num, user_num, arr);

    if (row_index != -1) {
        // Update existing entry
        arr[row_index][2]+=violations;
        return true; // Indicate successful update
    } else {
        // Add a new entry (if space is available)
        for (int i = 0; i < 1000; i++) {
            if (arr[i][0] == -1) { // Find an empty slot
                arr[i][0] = group_num;
                arr[i][1] = user_num;
                arr[i][2] = violations;
                return true; // Indicate successful addition
            }
        }
        return false; // No space available
    }
}
////////////////////////////////////////////////////////////////////////////////

struct chats
{
    long mtype;
    int user_id;
    int group_id;
    int timestamp;
    int no_of_violations;
    bool banned;
    char text_message[MAX_LINE_LENGTH];
};

int main(int argc, char*argv[])
{
    int x;
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
    int row=0;
    while(fgets(buffer[row],sizeof(buffer),file))
    {
        buffer[row][strcspn(buffer[row],"\n")]=0;
        row++;
    }
    fclose(file);
    int key=atoi(buffer[3]);
    int violations_threshold=atoi(buffer[4]);
    char filepath1[100];
    char buffer1[MAX_FILTERED_WORDS][MAX_WORD_LENGTH];
    snprintf(filepath1,sizeof(filepath1),"testcase_%d/%s",x,"filtered_words.txt");
    FILE*file1=fopen(filepath1,"r");
    if(file1==NULL)
    {
        perror("Error, file not found!\n");
        return 1;
    }
    int row1=0;
    while(fgets(buffer1[row1],sizeof(buffer1[row1]),file1))
    {
        buffer1[row1][strcspn(buffer1[row1],"\n")]=0;
        row1++;
    }
    fclose(file1);
    int filtered_count = 0;
    for (int i = 0; i < MAX_FILTERED_WORDS; i++) {
        if (strlen(buffer1[i]) > 0) {  // If the row is not empty
            filtered_count++;
        }
    }
    int user_group_pair[1000][3];
    struct chats message_new;
    message_new.mtype=1;
    int msgid=msgget(key,0666|IPC_CREAT);
        if (msgid == -1)
        {
            perror("Error in msgget");
            return 1;
        }
        ///////////////////////////////////////////////////////////////////
        for (int i = 0; i < 1000; i++) {
            for (int j = 0; j < 3; j++) {
                user_group_pair[i][j] = -1; 
            }
        }
        long startTime = getCurrentTime();
        //////////////////////////////////////////////////////////////
        while(1)
        {
            msgrcv(msgid, &message_new, sizeof(message_new) - sizeof(message_new.mtype), 100, 0);
            printf("%d\n",message_new.timestamp);
            startTime = getCurrentTime();
            char inputLower[MAX_INPUT_LENGTH];
            strcpy(inputLower, message_new.text_message);
            toLowerCase(inputLower);
            int threshold = 0;
            int found[MAX_FILTERED_WORDS] = {0};
            for (int i = 0; i < filtered_count; i++) {
                if (strstr(inputLower, buffer1[i]) != NULL) {  
                    if (!found[i]) {  // Count each unique violation once
                        threshold++;
                        found[i] = 1;
                    }
                }
            }
            //////////////////////////////////////////////////////////////
            add_or_update_data(message_new.group_id, message_new.user_id,threshold,user_group_pair);
            int get_row=find_row(message_new.group_id, message_new.user_id,user_group_pair);
            if(user_group_pair[get_row][2]>=violations_threshold)
            {
                message_new.banned=1;
            }
            message_new.no_of_violations=user_group_pair[get_row][2];
            message_new.mtype=message_new.group_id;












            //////////////////////////////////////////////////////////////////
            /*message_new.no_of_violations+=threshold;
            if(message_new.no_of_violations>=violations_threshold)
            {
                message_new.banned=true;
            }
            message_new.mtype=2;*/
            msgsnd(msgid, &message_new, sizeof(message_new) - sizeof(message_new.mtype), 0);
            if (getCurrentTime() - startTime >= TIMEOUT_SECONDS) {
                break;
            }
            sleep(1);
        }
        msgctl(msgid, IPC_RMID, NULL);
    return 0;
}