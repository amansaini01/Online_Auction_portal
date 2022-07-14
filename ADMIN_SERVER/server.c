#include "server_utilities.h"

Goods goods;            //Goods to auction
Goods goodslist[100];
int good_count = 0;     // Total number of goods in DB
int auction_state;      //Default = 0, after first person join = 1,
                      //during 1st 20s of bid = 2,during 2nd 20s of bid = 3, during 3rd 20s of bid = 4
int isCount;
User *users;
LogEntry logging[500];

int main(){
    int server_sockfd;  //Server socket descriptor
    int client_sockfd;  //Client socket descriptor
    int i;
    int command;        //Variable to receive command from client
    int byte_count;     //Number of bytes in the received queue
    int retval;         //return value

    int user_online = 0;  //number of user online
    int user_bidding =  0; // number of bidding
    int countdown;     //timeout countdown
    int log_number;     //Number of log entries
    int bid_money;      //money that client bid
    User *user;
    fd_set  readfds;    //read file descriptor list
    fd_set  allfds;     //all file descriptor list
    struct sockaddr_in server_address;  //Server address and port
    struct timeval timeout; //timeout value
    pthread_t tid;

    //Create server socket
    server_sockfd = socket(AF_INET,SOCK_STREAM,0);      // Create a socket
    if(server_sockfd == 0){
        perror("socket() error!\n");
        return 0;
    }
    //Create server address and port
    server_address.sin_family = AF_INET;                // Internet Protocol address family
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //convert host to network format (big endian byte order)
    server_address.sin_port = htons(8888);              // host-to-network-short: Swap endianess
    //Bind address and port to socket
    if(bind(server_sockfd, (struct sockaddr*)&server_address,sizeof(server_address))==-1){      //Bind socket address
        perror("bind() error!\n");
        close(server_sockfd);
        return -1;
    }
    //Server will listen with maximum connection
    if(listen(server_sockfd,10)==-1){                   // Start listening
        perror("listen() error!\n");
        close(server_sockfd);
        return -1;
    }
    FD_ZERO(&readfds);                                  // Initialize file descriptor
    FD_ZERO(&allfds);
    //Set server descriptor to descriptor list
    FD_SET(server_sockfd,&allfds);
    users = (User*)malloc(sizeof(User)*FD_SETSIZE);
loop:
    // Initial state of auction variables
    isCount = 0;
    goods.init_price = 0;
    goods.cur_price = 0;
    goods.min_incr = 0;
    // Menu thread

    //----------------------------------------------

    pthread_create(&tid, NULL, menuThread, NULL);
    // END MAIN MENU SECTION
     for(i = 0; i<FD_SETSIZE; i++){
        if (users[i].status == STT_BIDDING)
            users[i].status = STT_ONLINE;
        else {
            strcpy(users[i].name,"");
            strcpy(users[i].password,"");
            users[i].balance = 0;
            users[i].status = STT_OFFLINE;
        }
    }

    //Set timeout
    countdown = 60;     //timeout countdown
    log_number = 0;     //Number of log entries
    timeout.tv_sec = 1;
    timeout.tv_usec = 1000;
    //Set auction state as begin
    auction_state = 0;
    //Set number of log entry
    log_number = 0;

    while(1){
        //register all descriptor that might invoke activity
        readfds = allfds;
        //select() block calling process until there is activity on any of file descriptor
        retval = select(FD_SETSIZE,&readfds,NULL,NULL,&timeout);
        //An error occurs
        if(retval == -1){
            printf("select() error!\n");
            close(server_sockfd);
            return -1;
        }
        //Timeout period expired
        if(retval == 0){
            //Countdown
            if(countdown != 0){
                char line[100];
                if(auction_state > 0 && (countdown%10 == 0 || countdown == 5) && countdown != 60){
                    sprintf(line,"\033[F\033[F\r%d seconds left\033[K\n",countdown);
                    broadcast(line);
                }
                if (isCount == 1)
                {
                    countdown--;
                    printf("\r%2ds",countdown); fflush(stdout);
                }
            }
            else{
                char line[100];
                //Set 2nd countdown
                if(auction_state == 2){
                    countdown = 20;
                    auction_state = 3;
                    sprintf(line,"\r ******** %d RS 1st time ! *******\n\n\n",goods.cur_price);
                    printf("%s",line);
                    broadcast(line);
                //Set 3rd countdown
                } else if(auction_state == 3){
                    countdown = 20;
                    auction_state = 4;
                    sprintf(line,"\r ******** %d RS 2nd time ! *******\n\n\n",goods.cur_price);
                    printf("%s",line);
                    broadcast(line);
                } else if(auction_state == 4){
                    auction_state = 0;
                    if (log_number > 0)
                    {
                        sprintf(line,"\r ******** %s win %s with %d RS ! ********\n\n\n",users[logging[log_number-1].client].name, goods.name,logging[log_number-1].bid_money);
                        printf("%s",line);
                        broadcast(line);
                        //Update user balance and log
                        User winner = users[logging[log_number-1].client];
                        winner.balance -= logging[log_number-1].bid_money;
                        updateUser(winner);
                        time_t now;
                        time(&now);
                        writeLog(winner.name,goods.name,goods.cur_price,replace(ctime(&now),' ','.'));
                        //Inform end of this auction
                        broadcastEnd(logging[log_number-1].client);
                        write(logging[log_number-1].client,&winner.balance,sizeof(int));
                        printf("\nAuction ended successfully !\n");
                    }
                    else
                    {
                        printf("\nAuction failed. No one bid on this item.\n");
                    }
                    goto loop;
                } else {
                    printf("\nAuction failed. No one bid on this item.\n");
                    goto loop;
                }
            }
            //Reset timeout value cause it turns 0 after timeout
            timeout.tv_sec = 1;
        }

        //When an activity occurs
        for(i=0; i<FD_SETSIZE; i++){
            //Try all possible file descriptor value if it was the one which invoke activity
            if(FD_ISSET(i,&readfds)){
                //If server fd has activity, there is a connection request from new client
                if(i == server_sockfd){
                    //Accept new client file descriptor and set to file descriptor list
                    client_sockfd = accept(server_sockfd, NULL, NULL);
                    FD_SET(client_sockfd,&allfds);
                    printf("\rClient %d connected     \n",client_sockfd);
                    if (isCount ==0 ) { printf("\nYour option (1-4): "); fflush(stdout); }
                } else {
                    //Get number of byte in the received queue for a connection
                    ioctl(i,FIONREAD,&byte_count);
                    //If received queue for this connection is empty then it signals closing connection
                    if(byte_count == 0){
                        char line[100];
                        close(i);
                        FD_CLR(i,&allfds);
                        printf("\rClient %d closed     \n",i);
                        if (isCount ==0 ) { printf("\nYour option (1-4): "); fflush(stdout); }
                        //If user logged out, remove from array
                        if(strcmp(users[i].name,"")!=0){
                            printf("\rUser:%s has logged out\n",users[i].name);
                            if (isCount ==0 ) { printf("\nYour option (1-4): "); fflush(stdout); }
                            strcpy(users[i].name,"");
                            strcpy(users[i].password,"");
                            if(users[i].status == STT_BIDDING){
                                user_bidding--;
                                printf("\rBiding: %d users\t\t\n",user_bidding);
                            }
                            users[i].status = STT_OFFLINE;
                            user_online--;
                            printf("\rOnline: %d users\n",user_online);

                            // If user who has the higest bid quit, bid return to previous highest bid
                            if (isCount == 1)
                            {
                                while ((users[logging[log_number-1].client].status == STT_OFFLINE)&&(log_number > 0))
                                {
                                    log_number--;
                                }
                                if (logging[log_number-1].bid_money < goods.init_price) goods.cur_price = goods.init_price;
                                else goods.cur_price = logging[log_number-1].bid_money;
                                sprintf(line,"\rCurrent highest bid is now %d$.\n",goods.cur_price);
                                printf("%s",line);
                                broadcast(line);
                            }
                            else { printf("\nYour option (1-4): "); fflush(stdout); }
                        }
                    } else {
                        //Read command variable from client
                        read(i,&command,sizeof(int));
                        /****Register New User Process****/
                        if(command == CMD_REGISTER){
                            user = (User*)malloc(sizeof(User));
                            read(i,user,sizeof(User));
                            if(registerUser(user)!=1){
                                command = SIG_EXCEPTION;
                                free(user);
                                write(i,&command,sizeof(int));
                            } else {
                                strcpy(users[i].name,user->name);
                                strcpy(users[i].password,user->password);
                                users[i].balance = user->balance;
                                users[i].status = STT_ONLINE;
                                printf("\rUser:%s has logged in\n",users[i].name);
                                command = CMD_REGISTER;
                                user_online++;
                                printf("\rOnline: %d users\n",user_online);
                                if (isCount ==0 ) { printf("\nYour option (1-4): "); fflush(stdout); }
                                write(i,&command,sizeof(int));
                                write(i,&users[i].balance,sizeof(int));
                                write(i,getGoodsinfo(),sizeof(char)*100);
                            }
                         /***********Sign in*************/
                        } else if(command == CMD_SIGNIN){
                            user = (User*)malloc(sizeof(User));
                            read(i,user,sizeof(User));
                            if(authenticate(user)!=1){
                                command = SIG_EXCEPTION;
                                free(user);
                                write(i,&command,sizeof(int));  //send command
                            } else if(isLoggedIn(user->name)==1){
                                command = SIG_ALI;
                                free(user);
                                write(i,&command,sizeof(int));  //send command
                            } else {
                                strcpy(users[i].name,user->name);
                                strcpy(users[i].password,user->password);
                                users[i].balance = user->balance;
                                users[i].status = STT_ONLINE;
                                command = CMD_SIGNIN;
                                printf("\rUser: %s has logged in\n",users[i].name);
                                user_online++;
                                printf("Online: %d users\n",user_online);
                                if (isCount ==0 ) { printf("\nYour option (1-4): "); fflush(stdout); }
                                write(i,&command,sizeof(int));  //send command
                                write(i,&users[i].balance,sizeof(int)); //send balance
                                write(i,getGoodsinfo(),sizeof(char)*100);   //send good info
                            }
                        /***********Get History*********/
                        } else if(command == CMD_HISTORY){
                            write(i,getHistory(users[i].name),sizeof(char)*1024);
                        /***********Sign out*************/
                        } else if(command == CMD_SIGNOUT){
                            printf("\rUser:%s has logged out\n",users[i].name);
                            strcpy(users[i].name,"");
                            strcpy(users[i].password,"");
                            users[i].status = STT_OFFLINE;
                            user_online--;
                            printf("Online : %d users\n",user_online);
                            if (isCount ==0 ) { printf("\nYour option (1-4): "); fflush(stdout); }
                        /**********Join auction*******/
                        } else if(command == CMD_JOIN){
                            //change user status to bidding and update number of bidding users

                            if (goods.cur_price != 0)
                            {
                                users[i].status = STT_BIDDING;
                                user_bidding++;
                                printf("\r%s joins the auction\n",users[i].name);
                                printf("Biding: %d users\n",user_bidding);
                                //Reset time after first user join auction
                                if(auction_state == 0 && user_bidding == 1){
                                    auction_state = 1;
                                    printf("------------------------------------------\n");
                                    countdown = 60;
                                }
                                //char line[100];
                                //sprintf(line,"%d seconds left\n",countdown);
                                command = CMD_JOIN;
                                write(i,&command,sizeof(int));
                                write(i,&goods.cur_price,sizeof(int));
                                write(i,getGoodsinfo(),sizeof(char)*100);
                                //write(i,line,sizeof(char)*100);
                            }
                            else
                            {
                                command = CMD_REJECT;           // No auction, reject join request
                                write(i,&command,sizeof(int));
                            }
                        /*************Bidding************/
                        } else if(command == CMD_BID){
                            read(i,&bid_money,sizeof(int));
                            if(bid_money > users[i].balance){
                                command = SIG_NEM;                  // Account not having enough money to bid
                                write(i,&command,sizeof(int));
                            } else if ((bid_money>= (goods.cur_price+goods.min_incr))||((bid_money >= goods.init_price)&&(log_number == 0))){
                                char line[100];
                                //Set current price, logging and auction state
                                goods.cur_price = bid_money;
                                logging[log_number].client = i;
                                logging[log_number++].bid_money = bid_money;
                                auction_state = 2;
                                sprintf(line,"\r%s had bid %d RS for %s\n\n\n",users[i].name,bid_money,goods.name);
                                printf("%s",line);
                                command = CMD_BID;
                                write(i,&command,sizeof(int));
                                broadcast(line);
                                //Set 1st countdown
                                countdown = 21;
                            } else {
                                command = SIG_LOWBID;               // Bid less than current price + increment
                                write(i,&command,sizeof(int));
                            }
                        /**********Deposit*******/
                        //Add money to balance by searching for serial
                        } else if(command == CMD_DEPOSIT){
                            char serial[100];
                            int money;
                            read(i,serial,sizeof(char)*100);
                            money = deposit(users[i],serial);
                            if(money>0){
                                command = SIG_SUCCESS;
                                users[i].balance += money;
                                write(i,&command,sizeof(int));
                                write(i,&money,sizeof(int));
                            } else {
                                command = SIG_FAIL;
                                write(i,&command,sizeof(int));
                            }
                        }
                    }
                }
            }
        }
    }

    close(server_sockfd);
    return 0;
}
