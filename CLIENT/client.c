#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<string.h>
#include<netdb.h>
#include <fcntl.h>
#include <pthread.h>

#define SIG_FAIL      -6
#define SIG_SUCCESS   -5
#define SIG_LOWBID    -4
#define SIG_NEM       -3
#define SIG_ALI       -2
#define SIG_EXCEPTION -1
#define CMD_REGISTER   0
#define CMD_SIGNIN     1
#define CMD_JOIN       2
#define CMD_HISTORY    3
#define CMD_BID        4
#define CMD_SIGNOUT    5
#define CMD_END        6
#define CMD_WINNER     7
#define CMD_DEPOSIT    8
#define CMD_REJECT     9
#define STDIN          0

void menu();
char* getServerIP();
int menu1_option1();    //Sign in
int menu1_option2();    //Sign up
int menu2_option1();    //Get History
int menu2_option2();    //Join auction
int menu2_option3();    //Deposit money
int menu2_option4();    //Sign out

int sockfd; //socket of server file descriptor

typedef struct{
    char name[40];
    char password[50];
    int  balance;
}User;

User user;
int  price;

int main(){
    int     retval;
    int     option;
    int     menu_type=1;
    struct sockaddr_in address;
    //Create socket to connect to server
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == 0){
        perror("socket() error\n");
    }
    //Server ip address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(getServerIP());
    address.sin_port = htons(8888);

    retval = connect(sockfd,(struct sockaddr*)&address,sizeof(address));
    //Connection error
    if(retval == -1){
        perror("connect() error!\n");
        close(sockfd);
        return -1;
    }
    do{
        menu:
        menu(menu_type);
        scanf("%d",&option);
        getchar();
        switch(option){
            case 1:
                if(menu_type==1){
                    if(menu1_option1())
                        menu_type = 2;
                } else {
                    menu2_option1();
                }
                break;
            case 2:
                if(menu_type==1){
                    if(menu1_option2())
                        menu_type = 2;
                } else {
                    menu2_option2();
                    menu_type = 2;
                    goto menu;
                }
                break;
            case 3:
                if(menu_type==1){
                    close(sockfd);
                    return 0;
                } else {
                    menu2_option3();
                }
                break;
            case 4:
                if(menu_type==2){
                    if(menu2_option4()){
                        menu_type = 1;
                    }
                }
                else
                    printf("Please enter a valid option\n");
                break;
            default:
                printf("Please enter a valid option\n");
        }
    } while(1);
    return 0;
}

void menu(int type){
    if(type==1){
        printf("\n*** AUCTION CLIENT ***\n\n");
        printf("   1. Sign In\n");
        printf("   2. Sign Up\n");
        printf("   3. Exit\n\n");
        printf("Option: ");
    } else if(type == 2){
        printf("\n************************************\n");
        printf("\nHello! %s\n",user.name);
        printf("Your balance is %d RS\n\n", user.balance);
        printf("   1. Join Auction\n");
        printf("   2. History\n");
        printf("   3. Deposit money\n");
        printf("   4. Log Out\n\n");
        printf("Option: ");
    }
}

int menu1_option1(){    //Sign in
    int command;        //Variable to receive command from client
    User temp;
    char line[100];
    printf("Username:");    scanf("%s",temp.name);
    printf("Password:");    scanf("%s",temp.password);
    command = CMD_SIGNIN;
    write(sockfd,&command,sizeof(int));
    write(sockfd,&temp,sizeof(User));
    read(sockfd,&command,sizeof(int));
    if(command == SIG_EXCEPTION){
        printf("Incorrect username or password.\n");
        return 0;
    } else if(command == SIG_ALI){
        printf("Username %s already logged in!\n",temp.name);
        return 0;
    } else {
        //receive balance of user
        read(sockfd,&user.balance,sizeof(int));
        printf("\n************************************\n");
        printf("\nGoods for auction at the moment:\n");
        read(sockfd,line,sizeof(char)*100);
        printf("%s\n",line);
        strcpy(user.name,temp.name);
        strcpy(user.password,temp.password);
    }
    return 1;
}

int menu1_option2(){    //Sign up
    int command;        //Variable to receive command from client
    User temp;
    char password[50];
    char line[100];
    while(1){
        printf("New Username:");    scanf("%s",temp.name);
        if(strcmp(temp.name,"")==0){
            printf("Username must not be empty\n");
        } else {
            break;
        }
    }
    while(1){
        printf("Password:");        scanf("%s",temp.password);
        printf("Password Confirmation:");   scanf("%s",password);
        if(strcmp(temp.password,password)!=0){
            printf("Password and confirmation do not match.\n");
        } else {
            break;
        }
    }
    command = CMD_REGISTER;
    write(sockfd,&command,sizeof(int));
    write(sockfd,&temp,sizeof(User));
    read(sockfd,&command,sizeof(int));
    if(command != CMD_REGISTER){
        printf("Username already existed.\n");
        return 0;
    } else {
        printf("Register successfully!\n");
        printf("\nHello! %s\n",temp.name);
        printf("Goods for auction at the moment:\n");
        //receive balance of user
        read(sockfd,&user.balance,sizeof(int));
        read(sockfd,line,sizeof(char)*100);
        printf("%s",line);
        strcpy(user.name,temp.name);
        strcpy(user.password,temp.password);
    }
    return 1;
}

int menu2_option2(){    //Get History
    char line[1024];
    int command;
    command = CMD_HISTORY;
    write(sockfd,&command,sizeof(int));
    read(sockfd,line,sizeof(char)*1024);
    printf("%s\n",line);
    return 1;
}

int menu2_option1(){    //Join auction
    int command;
    char choice;
    char line[100];
    char buffer[200] = "";  //buffer for stdin
    int  retval;
    int  byte_count;
    int  bid_money;
    int  n;
    int warning = 0;
    fd_set allfds;
    fd_set readfds;
    command = CMD_JOIN;
    printf("You can't quit once you join the auction. Do you want to join?\n");
    printf("Join(1)/Cancel(any key)?:");    scanf("%c",&choice);
    if(choice == '1'){
        //Initiate read file descriptor
        FD_ZERO(&readfds);
        FD_ZERO(&allfds);
        FD_SET(sockfd,&allfds);
        FD_SET(STDIN,&allfds);
        //Send command, receive, price and goods infor
        write(sockfd,&command,sizeof(int));
        printf("Joining...\n");
        read(sockfd,&command,sizeof(int));
        if (command == CMD_REJECT)
        {
            printf("\n No auction is being held at the moment.\n\n");
            return 1;
        }
        read(sockfd,&price,sizeof(int));
        read(sockfd,line,sizeof(line));
        printf("%s",line);
        //read(sockfd,line,sizeof(line));
        //printf("%s",line);
        strcpy(line,"");
        printf("\n\n");
        printf("\nBid: "); fflush(stdout);
        while(1){
            readfds = allfds;
            retval = select(FD_SETSIZE,&readfds,NULL,NULL,NULL);
            if(retval == -1){
                perror("select() error\n");
                return -1;
            }
            //Activity triggered by server
            if(FD_ISSET(sockfd,&readfds)){
                ioctl(sockfd,FIONREAD,&byte_count);
                if(byte_count==0){
                    printf("Connection lost!\n");
                    exit(1);
                } else {
                    read(sockfd,line,sizeof(char)*100);
                    if(strlen(line) > 1){
                        printf("%s",line);
                    } else{
                        command = atoi(line);
                        if(command == CMD_END){
                            printf("\rEnd of auction !\n");
                            break;
                        } else if(command == CMD_WINNER){
                            read(sockfd,&user.balance,sizeof(int));
                            printf("\rEnd of auction !\n");
                            break;
                        }
                    }
                    printf("\nBid: "); fflush(stdout);


                }
              //Activity triggered by stdin
            } else if(FD_ISSET(STDIN,&readfds)){
                if ((n = read(STDIN, buffer, sizeof(char)*200)) != 0) {
                    bid_money = atoi(buffer);
                    if(bid_money==0){
                        printf("\rPlease enter a valid number\n");
                        warning += 1;
                        if (warning == 5)
                        {
                            printf("\r You have tried invalid input for 5 times, and forced to quit.\n");
                            exit(0);
                        }
                    } else {
                        command = CMD_BID;
                        write(sockfd,&command,sizeof(int));
                        write(sockfd,&bid_money,sizeof(int));
                        read(sockfd,&command,sizeof(int));
                        if(command == SIG_NEM){
                            printf("Your bidding money exceeds your balance\n");
                            warning += 1;
                            if (warning == 5)
                            {
                                printf("\r You have tried invalid input for 5 times, and forced to quit.\n");
                                exit(0);
                            }
                        } else if(command == SIG_LOWBID){
                            printf("Your bidding money is lower than current price + minimum increment\n");
                            warning += 1;
                            if (warning == 5)
                            {
                                printf("\r You have tried invalid input for 5 times, and forced to quit.\n");
                                exit(0);
                            }
                        }
                        else warning = 0;
                    }
                    printf("\nBid: "); fflush(stdout);
                }
            }

        }
    }
    return 1;
}

int menu2_option3(){    //Deposit money
    char serial[100];
    int command;
    int money;
    printf("Enter serial ID:");
    scanf("%s",serial);
    command = CMD_DEPOSIT;
    write(sockfd,&command,sizeof(int));
    write(sockfd,serial,sizeof(char)*100);
    read(sockfd,&command,sizeof(int));
    if(command == SIG_SUCCESS){
        read(sockfd,&money,sizeof(int));
        printf("%dRS has been deposited\n",money);
        user.balance += money;
    } else if(command == SIG_FAIL){
        printf("Your serial ID is not correct\n");
    }
    return 1;
}

int menu2_option4(){    //Sign out
    int command;
    command = CMD_SIGNOUT;
    printf("%s has logged out!\n",user.name);
    strcpy(user.name,"");
    strcpy(user.password,"");
    write(sockfd,&command,sizeof(int));
    return 1;
}

char* getServerIP()
{
    FILE *fi;
    char IP[17];

    if((fi = fopen("../DATABASE/server_addr.txt","r"))==NULL)  {
        printf("Error opening serverIP file!\n");
        return NULL;
    }

    fscanf(fi,"%s",IP);
    fclose(fi);
    return strdup(IP);

}
