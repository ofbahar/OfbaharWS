// Omer Faruk Bahar - 170418029 - BLM3010 - FinalÖdevi
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "sozluk.h"
#include "kuyruk.h"

// Soket yapısı tanımları
struct sockaddr_in adres;
int server_fd,gelen_istek,opt = 1,adres_len = sizeof(adres);

// Thread id değişkeni
pthread_t worker;

//IP ve Serve edilecek dizin tanımlandı
#define IP "127.0.0.1"
#define ROOT_FOLDER "web"

//HTTP Durum kodları ve içerikleri tanımlandı
const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

/**
 * SIGQUIT signal handler
 * Server soketi kapatır ve threadin sonlanmasını bekler
 *
 * @param signum sinyal türü.
 * @return void.
 */
void handler(int signum){

    printf("\nClosing sockets...");
    close(server_fd);
    pthread_join(worker,NULL);
    printf("\nOfbaharWS | github.com/ofbahar | Bye...\n");
    exit(1);
}

/**
 * Dosya uzantılarını döndüren fonksiyon
 *
 * @param filename uzantısı bulunacak dosya adı.
 * @return eğer uzantı yoksa "" döndürür
 * @return eğer uzantı varsa uzantıyı const char* olarak döndürür.
 */
const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

/**
 * HTTP istek başlığını işleyen method
 *
 * @param istek gelen istek başlığı.
 * @return eğer method GET değilse veya format bozuksa NULL döndürür
 * @return eğer GET ve format düzgünse istenilen dosya adını döndürür.
 */

char * http_baslik_istegi (const char * istek){

    if(istek == NULL)
       return NULL;

    char * metin = malloc(strlen(istek)+1);
    strcat(metin,istek);
    char * method = strtok(metin," ");
    if(strcmp(method,"GET") != 0)
        return NULL;
    char * dosya = strtok(NULL," ");
    char * path = malloc(strlen(dosya)+strlen(ROOT_FOLDER));

    strcpy(path,ROOT_FOLDER);

    if(dosya[0] == '/'){
        if(strcmp(dosya,"/") == 0)
            strcat(path,"/index.html");
        else
            strcat(path,dosya);
        return path;
    }else
        return NULL;

}
/**
 * HTTP isteklerini karşılayıp bu isteğe cevap veren thread fonksiyonu
 *
 * @param sock gelen soket bağlantısı
 * @return void.
 */

void *workerThread(void* sock){

    // pointer ve değişken tanımları
    char * extension;
    char * content;
    char * part = NULL;
    char buffer[2048],packet[2048];
    char *key,*value,*header;
    int fd;
    int soket = *(int*)sock;

    // Sözlük ve Queue tanımları
    dictionary_t dict;
    queue_t q;

    // Sözlük ve Queue yapıları başlatılıyor
    dictionary_init(&dict);
    queue_init(&q);

    // Gelen ve gidecek paket içeriğini tutacak diziler temizleniyor
    memset(packet,0,2048);
    memset(buffer,0,2048);

    // Client soketinden paket alınıyor...
    int paket = recv(soket, buffer,2048,0);

    // Paket düzgün alınamadıysa thread sonlandırılıyor...
    if(paket == -1){
        printf("Paket Alinamadi!\n");
        pthread_exit(NULL);
    }

    // Gelen paket ekrana yazdırılıyor...
    printf("GELEN PAKET\n--------------\n%s",buffer);

    // Gelen paketteki headerler satır satır ayrılıp queue yapısına aktarılıyor
    printf("\nPARCALANIYOR\n--------------");

    part = strtok(buffer,"\r\n");
    header = part;
    printf("\nHTTP Baslik istegi : %s\n",header);
    while(part != NULL){

        part = strtok(NULL,"\r\n");
        printf("Part : %s\n",part);
        queue_enqueue(&q,part);
    }

    // Header sayısı ekrana yazdırılıyor
    int headerCount = queue_size(&q);
    printf("Toplam Baslik : %d\n\n",headerCount);

    // Ayrılan headerler bir daha ayrılıp key,value şeklinde sözlük yapısına aktarılıyor
    if(headerCount > 0)
    for(int i = 0;i < headerCount-1; i++){

        part = queue_at(&q,i);
        key = strtok(part,":");
        value = strtok(NULL,"\0");
        dictionary_add(&dict,key,value+1);
    }

    // Baslik isteği işlenip dosya yolu alınıyor
    char * path = http_baslik_istegi(header);

    // Başlık isteği işlenemediyse client'a 501 http durum kodu gönderiliyor
    if(path == NULL){

        sprintf(packet,"HTTP/1.1 501 %s\r\nServer: OfbaharWS/1.0\r\nConnection: close\r\n\r\n",HTTP_501_STRING);
        strcat(packet,HTTP_501_CONTENT);
        send(soket,packet,strlen(packet),0);

    // Başlık işlenip path geldiyse
    }else{
	// Dosya uzantısı alınıyor
        extension = malloc(strlen(path));
        strcat(extension,get_filename_ext(path));

	// Dosya boyutu alınıyor
        struct stat st;
        stat(path, &st);
        size_t size = st.st_size;

	// Dosya okumak için açılıyor
        if((fd = open(path, O_RDONLY)) != -1 ){

	    // Dosya okunuyor
            content = malloc(size);
            read(fd,content,size);

	    // Dosya türlerine göre gidecep response paketleri hazırlanıyor
            if(strcmp(extension,"html") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: text/html\r\n",HTTP_200_STRING,size);
            else if(strcmp(extension,"css") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: text/css\r\n",HTTP_200_STRING,size);
            else if(strcmp(extension,"js") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: text/javascript\r\n",HTTP_200_STRING,size);
            else if(strcmp(extension,"png") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: image/png\r\n",HTTP_200_STRING,size);
            else if(strcmp(extension,"jpeg") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: image/jpeg\r\n",HTTP_200_STRING,size);
            else if(strcmp(extension,"ico") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: image/ico\r\n",HTTP_200_STRING,size);
            else if(strcmp(extension,"mp3") == 0)
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: audio/mpeg\r\n",HTTP_200_STRING,size);
            else
                sprintf(packet,"HTTP/1.1 200 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %zu\r\nContent-Type: text/plain\r\n",HTTP_200_STRING,size);

	    // Connection headeri gelmemiş ise bağlantıyı close olarak ayarlıyoruz
            if(dictionary_get(&dict,"Connection") == NULL){
                strcat(packet,"Connection: close\r\n\r\n");
            }

            // Connection headeri gelmiş ve değeri keep-alive ise bağlantıyı keep-alive olarak ayarlıyoruz
            else if(strcasecmp(dictionary_get(&dict,"Connection"),"keep-alive") == 0){

                strcat(packet,"Connection: keep-alive\r\n\r\n");

            }

            // Connection headeri gelmiş ve değeri close ise bağlantıyı close olarak ayarlıyoruz
            else{
                strcat(packet,"Connection: close\r\n\r\n");
            }

	    // İlk önce header paketini daha sonra dosya içeriğini gönderiyoruz
            send(soket,packet,strlen(packet),0);
            send(soket,content,size,0);

	// Eğer dosya açılamazsa 404 dönüyoruz
        }else{

            sprintf(packet,"HTTP/1.1 404 %s\r\nServer: OfbaharWS/1.0\r\nContent-Lenght: %ld\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n",HTTP_404_STRING,strlen(HTTP_404_CONTENT));
            send(soket,packet,strlen(packet),0);
            send(soket,HTTP_404_CONTENT,strlen(HTTP_404_CONTENT),0);

        }

    }

    printf("Gonderilen Paket\n------------------\n%s",packet);

    // Bağlantı kapatılıp threadden çıkılıyor...
    close(soket);
    pthread_exit(NULL);
}

/**
 * Komut satrından girilen port değerini belirlenen ip veya domain adresine bind edip,
 * soket bağlantısı açarak gelen bağlantıları worker threadlere aktaran http web server.
 *
 * @param argc komut satırı argüman sayısı
 * @param argv komut satırı argümanları
 * @return exit kodu.
 */

int main(int argc, char *argv[]){

    // Programa port girilmemişse usage yazdırılıyor
    if(argc != 2){

        printf("Usage : %s <PORT>\n",argv[0]);
        exit(1);

    }

    // Signal handler ayarlanıyor
    struct sigaction action;
    action.sa_handler = handler;
    sigaction(SIGQUIT, &action, NULL);


    // Soket oluşturuluyor
    if((server_fd = socket(AF_INET, SOCK_STREAM,0)) == 0){
        perror("Soket olusturulamadi!\n");
        exit(1);
    }

    // Soket seçenekleri ayarlanıyor
    if(setsockopt(server_fd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("Soket ayarlanamadi!\n");
        exit(1);
    }

    // Bind için ip ve port alınıyor
    adres.sin_family =  AF_INET;
    inet_aton(IP,&adres.sin_addr);
    adres.sin_port = htons(atoi(argv[1]));

    // Bind işlemi gerçekleşiyor
    if(bind(server_fd,(struct sockaddr *)&adres,sizeof(adres)) < 0){
        perror("Bind islemi basarisiz\n");
        exit(1);
    }

    // Gelen bağlantılar dinlenmeye başlıyor
    if(listen(server_fd,10) < 0){

        perror("Soket dinleme islemi basarisiz!\n");
        exit(1);

    }

    while(1){

	// Gelen bağlantı alınıyor
        if((gelen_istek = accept(server_fd,(struct sockaddr *)&adres,(socklen_t*)&adres_len)) < 0){
            perror("Gelen baglanti istegi basarisiz!\n");
            exit(1);
        }

	// Gelen soket threade aktarılıyor
	char *ip = inet_ntoa(adres.sin_addr);
	uint16_t port = ntohs(adres.sin_port);
	printf("IP : %s:%u\n",ip,(unsigned int)port);
	//printf("Gelen bağlantı : %s:%d\n",inet_ntoa(adres.sin_addr),(int) ntohs(adres.sin_port));
        pthread_create(&worker,NULL,workerThread,(void *)&gelen_istek);

    }

    return 0;

}
