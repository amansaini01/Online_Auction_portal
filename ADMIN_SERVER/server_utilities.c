#include "server_utilities.h"

extern Goods goods;            //Goods to auction
extern Goods goodslist[100];
extern int good_count;     // Total number of goods in DB
extern int auction_state;      //Default = 0, after first person join = 1,
                        //during 1st 20s of bid = 2,during 2nd 20s of bid = 3, during 3rd 20s of bid = 4
extern int isCount;
extern User *users;
extern LogEntry logging[500];

int registerUser(User* user){
    FILE *file;
    if(getUserByName(user->name) != NULL){
        return 0;
    }
    if((file = fopen("../DATABASE/bidders.txt","a"))==NULL)  {
        printf("Error opening file!\n");
        return -1;
    }
    user->balance = 500000;
    fprintf(file,"%s\t%s\t%d\n",user->name,user->password,user->balance);
    fclose(file);
    return 1;
}

int updateUser(User user){
    FILE *file;
    User temp[100];
    int n = 0;
    int i;
    if((file = fopen("../DATABASE/bidders.txt","r"))==NULL)  {
        printf("Error opening file!\n");
        return 0;
    }
    while(fscanf(file,"%s %s %d",temp[n].name,temp[n].password,&temp[n].balance)!= EOF){
        if(strcmp(user.name,temp[n].name)==0){
            temp[n].balance = user.balance;
        }
        n++;
    }
    fclose(file);
    if((file = fopen("../DATABASE/bidders.txt","w"))==NULL)  {
        printf("Error opening file!\n");
        return 0;
    }
    for(i = 0; i<n; i++){
        fprintf(file,"%s\t%s\t%d\n",temp[i].name,temp[i].password,temp[i].balance);
    }
    fclose(file);
    return 1;
}

int authenticate(User *user){
    User* temp;
    temp = getUserByName(user->name);
    if(temp==NULL)  return 0;
    if(strcmp(user->password,temp->password)==0){
        user->balance = temp->balance;
        return 1;
    }
    return 0;
}

void broadcastEnd(int winner){
    int i;
    int command = CMD_END;
    char line[100];
    sprintf(line,"%d",command);
    for(i = 0; i<FD_SETSIZE; i++){
        if(users[i].status == 2){
            if(i!=winner){
                write(i,line,sizeof(char)*100);
            }
            else{
                command = CMD_WINNER;
                sprintf(line,"%d",command);
                write(i,line,sizeof(char)*100);
                command = CMD_END;
                sprintf(line,"%d",command);
            }
        }
    }
}

void broadcast(char* str){
    int i;
    for(i = 0; i<FD_SETSIZE; i++){
        if(users[i].status == 2){
            write(i,str,sizeof(char)*100);
        }
    }
}

User* getUserByName(char *name){
    FILE *file;
    User* user;
    if((file = fopen("../DATABASE/bidders.txt","r"))==NULL)  {
        printf("Error opening file!\n");
        return NULL;
    }
    user = (User*)malloc(sizeof(User));
    while(fscanf(file,"%s %s %d",user->name,user->password,&user->balance)!= EOF){
        if(strcmp(user->name,name)==0){
            return user;
        }
    }
    fclose(file);
    return NULL;
}

int isLoggedIn(char* name){
    int i;
    for(i = 0; i<FD_SETSIZE; i++){
        if(strcmp(name,users[i].name)==0)
            return 1;
    }
    return 0;
}

char* getGoodsinfo(){
    char line[100];
    if (goods.cur_price == 0) sprintf(line,"\n  No auction is being held at the moment.\n");
    else sprintf(line,"  Goods: \t\t%s\n  Initial Price: \t%d RS\n  Current Price: \t%d RS\n  Step Size: \t\t%d RS\n",goods.name,goods.init_price,goods.cur_price,goods.min_incr);
    line[strlen(line)-1] = '\0';
    return  strdup(line);
}

int MenuChoice()
{
    int choice = 0;
    printf("**** AUCTION CONTROL MAIN MENU **** ");
    printf("\n\n");
    printf("1. Choose item to start auction.\n");
    printf("2. Edit item database.\n");
    printf("3. View auction history.\n");
    printf("4. Exit.\n");
    printf("\n Your choice (1-4): ");
    scanf("%d",&choice);
    while (!((choice >=1)&&(choice <=4)))
    {
        printf("\n Invalid choice. Please input a number from 1 to 4: ");
        while ( getchar() != '\n' );
        scanf("%d",&choice);
    }
    return choice;
}

void importDB()
{
    FILE *fi;
    int n = 0;
    Goods tmp;

    if((fi = fopen("../DATABASE/all_items.txt","r"))==NULL)  {
        printf("Error opening goods file!\n");
        return;
    }

    while (fscanf(fi,"%d\t%d\t",&tmp.init_price,&tmp.min_incr)>0)
        {
            fgets(tmp.name,40,fi);
            tmp.name[strlen(tmp.name)-1]='\0';
            goodslist[n].init_price = tmp.init_price;
            goodslist[n].min_incr = tmp.min_incr;
            strcpy(goodslist[n].name, tmp.name);
            n++;
        }
    fclose(fi);
    good_count = n;
}

void exportDB()
{
    FILE *fo;
    int i=0;
    if((fo = fopen("../DATABASE/all_items.txt","w"))==NULL)  {
        printf("Error opening goods file!\n");
        return;
    }

    for (i=0;i<good_count;i++)
            fprintf(fo,"%d\t%d\t%s\n",goodslist[i].init_price,goodslist[i].min_incr,goodslist[i].name);

    fclose(fo);
}


int writeLog(char *name, char* goods, int price,char* datetime){
    FILE *file;
    if((file = fopen("../DATABASE/history.txt","a+"))==NULL)  {
        printf("Error opening file!\n");
        return 0;
    }
    fprintf(file,"%s\t%s\t%d\t%s",name,goods,price,datetime);
    fclose(file);
    return 1;
}

int deposit(User user,char* serial){
    FILE *file;
    char str[100];
    int flag = 0;
    int money = 0;
    if((file = fopen("../DATABASE/serial_list.txt","r"))==NULL)  {
        printf("Error opening file!\n");
        return 0;
    }
    while(fscanf(file,"%s %d",str,&money)!= EOF){
        if(strcmp(serial,str)==0){
            flag = 1;
            break;
        }
    }
    fclose(file);
    if(!flag){
        return 0;
    }
    user.balance += money;
    updateUser(user);
    return money;
}

void printGoodList()
{
    int i=0;

    printf("\n");
    printf("|-----|------------------------------------|---------------|---------------|\n");
    printf("| No. |              Item name             |  Start price  | Min increment |\n");
    printf("|-----|------------------------------------|---------------|---------------|\n");
    if (good_count < 1)
    printf("|                        There is no item in database !                    |\n");
    for (i=0; i<good_count; i++)
        printf("| %3d | %-34s | %11d   | %11d   |\n",i+1,goodslist[i].name,goodslist[i].init_price,goodslist[i].min_incr);
    printf("|-----|------------------------------------|---------------|---------------|\n");
}

int chooseAuctionItem()
{
    int k = 0;
    printf("\n");
    if (good_count < 1)
        {
            printf(" Press add an item to the database first !\n");
            return 0;
        }

    printf("    Input No. of item to auction: (1-%d): ",good_count); scanf("%d",&k);
    while ((k < 1) || (k > good_count))
    {
        printf("Invalid item No. Please input a number from 1 to %d : ",good_count);
        scanf("%d",&k);
    }

    goods.init_price = goodslist[k-1].init_price;
    goods.min_incr = goodslist[k-1].min_incr;
    goods.cur_price = goods.init_price;
    strcpy(goods.name, goodslist[k-1].name);
    printf("Item '%s' with start price %d$ and minimum increment bid %d$ is selected !\n",goods.name,goods.init_price,goods.min_incr);
    return 1;
}

int editMenu()
{
    int choice = 0;
    printf("**** EDITTING ITEM DATABSE **** ");
    printf("\n\n");
    printf("1. Add new item.\n");
    printf("2. Edit item.\n");
    printf("3. Delete item.\n");
    printf("4. Back to main menu.\n");
    printf("\n Your choice (1-4): ");
    scanf("%d",&choice);
    while (!((choice >=1)&&(choice <=4)))
    {
        printf("\n Invalid choice. Please input a number from 1 to 4: ");
        while ( getchar() != '\n' );
        scanf("%d",&choice);
    }
    return choice;
}

void enterGoods(){
    importDB();
    printf("** Enter detail of new item **\n\n");
    while ( getchar() != '\n' );
    printf("Name of the goods: ");      gets(goodslist[good_count].name);
    printf("Initial price: ");          scanf("%d",&goodslist[good_count].init_price);
    printf("Minimum increment:");       scanf("%d",&goodslist[good_count].min_incr);
    good_count ++;
    exportDB();
    printf("\nItem added to database successfully !\n");
    printGoodList();
}

void editGoods(){
    importDB();
    int choice = 0;
    printGoodList();
    printf("\n\n");
    printf("**** Enter No. of the item to edit ****");
    printf("\n\n");
    scanf("%d",&choice);
    while (!((choice >=1)&&(choice <=good_count)))
    {
        printf("\n Invalid choice. Please input a number from 1 to %d: ",good_count);
        scanf("%d",&choice);
    }
    printf("\n");
    while ( getchar() != '\n' );
    printf("New name of the goods: ");      fgets(goodslist[choice-1].name,50,stdin);   goodslist[choice-1].name[strlen(goodslist[choice-1].name)-1]='\0';
    printf("New initial price: ");          scanf("%d",&goodslist[choice-1].init_price);
    printf("New minimum increment:");       scanf("%d",&goodslist[choice-1].min_incr);

    exportDB();
    printf("\nItem editted successfully !\n");
    printGoodList();
}

void deleteGoods(){
    importDB();
    int i = 0, choice = 0;
    printGoodList();
    printf("\n\n");
    printf("**** Enter No. of the item to delete ****");
    printf("\n\n");
    scanf("%d",&choice);
    while (!((choice >=1)&&(choice <=good_count)))
    {
        printf("\n Invalid choice. Please input a number from 1 to %d: ",good_count);
        scanf("%d",&choice);
    }
    printf("\n");
    good_count--;
    for (i = choice-1; i < good_count; i++)
    {
        goodslist[i].init_price = goodslist[i+1].init_price;
        goodslist[i].min_incr = goodslist[i+1].min_incr;
        strcpy(goodslist[i].name, goodslist[i+1].name);
    }

    exportDB();
    printf("\nItem deleted successfully !\n");
    printGoodList();
}

char* getHistory(char* user_name){
    char history[1024];
    char line[100];
    char name[40];
    char goods_name[50];
    char price[10];
    char* datetime = (char*)malloc(sizeof(char)*30);
    FILE *file;
    if((file = fopen("../DATABASE/history.txt","r"))==NULL)  {
        printf("Error opening file!\n");
        return NULL;
    }
    sprintf(history,"\rGoods                         \tPrice     \tDate&Time\n");
    while(fscanf(file,"%s %s %s %s",name,goods_name,price,datetime)!= EOF){
        if(strcmp(name,user_name)==0){
            datetime = replace(datetime,'.',' ');
            strcat(price," RS");
            sprintf(line,"%-30s\t%-10s\t%s\n",goods_name,price,datetime);
            strcat(history,line);
        }
    }
    fclose(file);
    return strdup(history);
}

char* replace(char* str,char old, char new){
    int n = strlen(str);
    int i;
    char* str1 = strdup(str);
    for(i=0; i<n; i++){
        if(str1[i]==old){
            str1[i] = new;
        }
    }
    return str1;
}


void viewHistory()
{
    FILE *fi;
    char line[100];

    if((fi = fopen("../DATABASE/history.txt","r"))==NULL)  {
        printf("Error opening auction history file!\n");
        return;
    }

    printf("\n\n");
    printf("**** Auction History ****");
    printf("\n\n");

    while (fgets(line,40,fi)>0)
    {
        printf("%s",line);
    }
    printf("\n\n");
    fclose(fi);
}

void* menuThread(void* threadid)
{   int choice = 0;

main_menu:
    choice = MenuChoice();
    switch (choice)
    {
        case 1: // Choose item, then start auction
            importDB();
            printGoodList();
            if (chooseAuctionItem() == 0) goto main_menu;
             do{
                printf("\n***Press Enter to immediately begin auction, or input 'b' to back to menu***\n");
                while ( getchar() != '\n' );
                choice = getchar();
                if (choice == 'b') goto main_menu;
             }
             while(choice!='\n');
             isCount = 1;
            break;
        case 2: // Edit item database
            edit_menu:
            choice = editMenu();
            switch (choice)
            {
                case 1: enterGoods(); goto edit_menu; break;
                case 2: editGoods(); goto edit_menu; break;
                case 3: deleteGoods(); goto edit_menu; break;
                case 4: goto main_menu;
            }
            break;
        case 3: // View auction history
            viewHistory();
            goto main_menu;
            break;
        case 4: // Exit
            exit(0); break;
    }
    return NULL;
}

