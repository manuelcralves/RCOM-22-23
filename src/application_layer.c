// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "utils.h"
#include <math.h>
#include <sys/stat.h>

extern int fd;

LinkLayerRole getRole(const char *role)
{
  if (strcmp(role, "tx") == 0)
    return LlTx;
  else if (strcmp(role, "rx") == 0)
    return LlRx;
  else
  {
    printf("Invalid Role!\n");

    return (LinkLayerRole)NULL;
  }
}

int appRead (int fd, LinkLayer ll, const char *name) {
    unsigned char *buffer = malloc(1000);
    int res, currentIndex = 0;
    off_t fileLengthStart = 0, currentPosition = 0, dataLength = 0, fileLengthEnd = 0;
    unsigned int nameSize = 0;

    int open = llopen(ll);
    if (open == -1) return;

    FILE *receptorFptr = fopen(name,"wb");

    /*Receive start packet*/
    res = llread(buffer);
    if (res<0) {
        perror("llread");
        exit(-1);
    }

    for (int i = 0; i <18;i++){printf("%x\n",buffer[i]);}

    if (buffer[currentIndex++] != START_PKT){
        perror("start packet");
        exit(-1);
    }

    /*File length*/
    if (buffer[currentIndex++] != FILE_SIZE){
        perror("start-wrong type");
        exit(-1);
    }

    int length = buffer[currentIndex++];

    for (size_t i = 0; i < length;i++){
        fileLengthStart = (fileLengthStart*256) + buffer[currentIndex++]; /* hexa to dec*/
    }

    /*File name*/

    if (buffer[currentIndex++] != FILE_NAME){
        perror("start-wrong type");
        exit(-1);
    }

    nameSize = buffer[currentIndex++];
    char *nameToCompare = (char *)malloc(nameSize);
    for (size_t i = 0; i < nameSize;i++){
        nameToCompare[i] = buffer[currentIndex++];
    }
    

    while (currentPosition != fileLengthStart){
        if ((res = llread(buffer)) < 0) {
            free(nameToCompare);
            fclose(receptorFptr);
            perror("llread");
            exit(-1);
        }

        if (!res) continue;

        currentIndex = 0;
        if (buffer[currentIndex++] != DATA_PKT) {
            free(nameToCompare);
            fclose(receptorFptr);
            perror("FIle not fully received");
            exit(-1);
        }

        currentIndex++;
        dataLength = buffer[currentIndex] * 256 + buffer[currentIndex+1];

        currentIndex += 2;
        char * data = (char *)malloc(dataLength);
        for(size_t i = 0; i < dataLength;i++){
            data[i] = buffer[currentIndex++];
        }

        currentPosition += dataLength;

        if (fwrite(data,dataLength,1,receptorFptr) < 0){
            free(data);
            free(nameToCompare);
            fclose(receptorFptr);
            perror("Write to file");
            exit(-1);
        }

        free(data);

    }

    /*Receive end packet*/
    currentIndex = 0;
    if ((res = llread(buffer)) < 0) {
        free(nameToCompare);
        perror("llread");
        exit(-1);
    }

    if (buffer[currentIndex++] != END_PKT){
        free(nameToCompare);
        perror("WRong type");
        exit(-1);
    }

    /*File length*/
    length = buffer[currentIndex++];
    for (size_t i = 0; i < length;i++){
        fileLengthEnd = (fileLengthEnd * 256) + buffer[currentIndex++]; // hexa to dec
    }

    if (fileLengthEnd != fileLengthStart){
        free(nameToCompare);
        perror("Wrong length");
        exit(-1);
    }   

    /*File name*/
    if(buffer[currentIndex++] != FILE_NAME){
        free(nameToCompare);
        perror("End - Wrong type");
        exit(-1);
    }

    if (buffer[currentIndex++] != nameSize){
        free(nameToCompare);
        perror("Wrong name size");
        exit(-1);
    }

    for (size_t i = 0; i < nameSize;i++){
        if (nameToCompare[i] != buffer[currentIndex++]){
            free(nameToCompare);
            perror("Wrong name");
            exit(-1);
        }
    }

    free(nameToCompare);
    if (llclose(fd) < 0) {
        return -1;
    }
    return 0;

}

int appWrite(int fd, LinkLayer ll,const char * name) {

    unsigned int currentIndex = 0;
    int resW = 0, resR = 0, seqNum = 0;
    unsigned char countBytes = 0;


    FILE *transmissorFptr = fopen(name,"rb");

    if (transmissorFptr ==NULL){
        printf("Can't open file!\n");
        exit(1);
    }

    fseek (transmissorFptr,0L,SEEK_END); // 0L = 0 long int, EOF

    int open = llopen(ll);
    if (open == -1) return;

    long int fileSize = ftell(transmissorFptr);
    long int auxFileSize = fileSize;

    fseek(transmissorFptr,0,SEEK_SET); //beginning of file

    while (auxFileSize > 0) {
        countBytes++;
        auxFileSize /= 255;
    }

    unsigned char controlPacket[255];
    controlPacket[currentIndex++] = START_PKT; /* Control */
    controlPacket[currentIndex++] = FILE_SIZE; /* Type*/
    controlPacket[currentIndex] = countBytes; /*Length , number of bytes needed to represent the file length*/

    for (size_t i = controlPacket[currentIndex++]; i > 0;i--){
        controlPacket[currentIndex++] = fileSize & 0xFF;
        fileSize = fileSize >> 8;
    }

    controlPacket[currentIndex++] = FILE_NAME;
    controlPacket[currentIndex++] = strlen(name);
    for (size_t i = 0; i < strlen(name);i++){
        controlPacket[currentIndex++] = name[i];
    }

    printf("vai escrever controlPacket com file size, file name , tamanho : %d\n",currentIndex);
    /* Send start packet*/
    llwrite(controlPacket,currentIndex);
    printf("ja escreveu control packet");
    unsigned char *data = malloc(500);
    off_t currentPosition = 0;

    fseek(transmissorFptr,0,SEEK_SET);
    /* Send data packets */
    
    while (currentPosition != fileSize) {
        if ((resR = fread(data+4,1,496,transmissorFptr)) < 0) {
            free(data);
            exit(-1);
        }
        printf("resR do fichiero penguin.gif: %d",resR);
        data[0] = DATA_PKT;
        data[1] = seqNum % 255;
        data[2] = (resR/256);
        data[3] = (resR % 256);

        if((resW = llwrite(data,(256*data[2] + data[3]) + 4)) < 0){
            free(data);
            exit(-1);
        }

        currentPosition += resR;
        seqNum++;
    }

    controlPacket[0] = END_PKT;
    llwrite(controlPacket,currentIndex);
    free(data);

    if (llclose(fd) < 0) {
        return -1;
    }
    
    return 0;

}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{   

    LinkLayer ll = {.baudRate = baudRate, .nRetransmissions = nTries, .timeout = timeout};
    strcpy(ll.serialPort,serialPort);

    

    if (strcmp(role,"rx") == 0) {
        LinkLayerRole llrole = LlRx;
        ll.role = llrole;
        printf("role rec\n");
        appRead(fd,ll,filename);
    }

    else if (strcmp(role,"tx")==0){
        LinkLayerRole llrole = LlTx;
        ll.role = llrole;
        printf("role trans\n");
        appWrite(fd,ll,filename);
    }

}
