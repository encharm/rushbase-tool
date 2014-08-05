#include <stdio.h>
#include <string.h>

#define CURL_STATICLIB
#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

#undef curl_easy_getinfo

struct myprogress {
  double lastruntime;
  CURL *curl;
  bool finished;
  
  double speed;
  curl_off_t lastcounter;
  double lastspeedtime;
  
  int counter;
};

static double format_speed(double speed, char* format)
{
    if(speed > 1024*1024*1024) {
        strcpy(format, "GB/s");
        return speed / (1024*1024*1024);
    }
    if(speed > 1024*1024) {
        strcpy(format, "MB/s");
        return speed / (1024*1024);
    }
    if(speed > 1024) {
        strcpy(format, "KB/s");
        return speed / (1024);
    }
    strcpy(format, "B/s");
    return speed;
}

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  double curtime = 0;
 
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
 
  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */ 

    if((curtime - myp->lastruntime > 0.5 || ulnow == ultotal) && ultotal != 0 && !myp->finished) {
        if(myp->counter % 2 == 0) {
            
            double t = curtime - myp->lastspeedtime;
            myp->speed = (ulnow - myp->lastcounter) / t;
            
            
            myp->lastcounter = ulnow;
            myp->lastspeedtime = curtime;
        }
        char format_str[16] = {0,};
        double speed = format_speed(myp->speed, format_str);
        
        fprintf(stderr, "\rProgress: %6.1f %% @ %6.1f %4s", (double)100*ulnow/ultotal, speed, format_str);
        myp->lastruntime = curtime;
        myp->counter++;
    }
    if(ulnow == ultotal && ulnow != 0 && !myp->finished) {
        myp->finished = true;
        char format_str[16] = {0,};
        double speed = format_speed(curtime ? ultotal/curtime : 0, format_str);
        fprintf(stderr, "\rAll uploaded @ %6.1f %4s avg     \n", speed, format_str);
    }
 
  return 0;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    CURL* curl = (CURL*)userp;
    int http_code;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if(http_code != 200) {
        fprintf(stderr, "Status: ");
        fwrite(buffer, size, nmemb, stderr);
        fputc('\n', stderr);
    }
    return size * nmemb;
}

static void printhelp(const char* name)
{
    printf("Usage: %s <options> [<file1> ... <fileN>]\n", name);
    printf("Options:  --help        -h  Print this help\n");
    printf("          --destination -d  Destination (e.g. ut.rushbase.net/<user>/dir)\n");
    printf("                            (may be set via environment RUSHBASE_DESTINATION)\n");
    printf("          --username    -u  Set username\n");
    printf("                            (may be set via environment RUSHBASE_USERNAME)\n");
    printf("          --password    -p  Set password\n");
    printf("                            (may be set via environment RUSHBASE_PASSWORD)\n");
    printf("Operations:\n");
    printf("          --mkdir <name> -m <name>  Create directory\n");
    printf("          --rm    <name> -r <name>  Remove file\n");
//    printf("          --save        -s  Save username and password\n");
//    printf("                            (to $HOME/.<destination's domain>)\n");   
}

int main(int argc, char *argv[])
{
    int c;
    char username[64] = {0,};
    char password[64] = {0,};
    char destination[4096] = {0,};
    static int verbose_flag;
    static int save_user_pass;
    
    if(getenv("RUSHBASE_USERNAME")) {
        strncpy(username, getenv("RUSHBASE_USERNAME"), sizeof(username)-1);
    }
    if(getenv("RUSHBASE_PASSWORD")) {
        strncpy(password, getenv("RUSHBASE_PASSWORD"), sizeof(password)-1);
    }
    if(getenv("RUSHBASE_DESTINATION")) {
        strncpy(destination, getenv("RUSHBASE_DESTINATION"), sizeof(destination)-1);
    }
    
    curl_global_init(CURL_GLOBAL_ALL);
        
#define assure_params() \
    if(!username[0] || !password[0]) { \
        fprintf(stderr, "Error: Username and password must be set!\n"); \
        printhelp(argv[0]); \
        return 1; \
    } \
    if(!destination[0]) { \
        fprintf(stderr, "Error: destination must be set!\n"); \
        printhelp(argv[0]); \
        return 1; \
    }

    
    void make_simple_request (const char* method, const char* field, const char* value) {
        struct curl_httppost *formpost=NULL;
        struct curl_httppost *lastptr=NULL;
        struct curl_slist *headerlist=NULL;
        static const char buf[] = "Expect:";
        
        CURL *curl = curl_easy_init();
        char url[4096+10];
        snprintf(url, sizeof(url), "%s%s", (strncmp(destination, "https://", 8) == -1 && strncmp(destination, "http://", 7) == -1)?"https://":"", destination);
    
        curl_easy_setopt(curl, CURLOPT_URL,url);
        
        curl_easy_setopt(curl, CURLOPT_USERNAME, username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);
        
        curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, field,
                CURLFORM_COPYCONTENTS, value,
                CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        
        CURLcode res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "Error: %s\n",
                curl_easy_strerror(res));
        
        curl_easy_cleanup(curl);
        curl_formfree(formpost);
        curl_slist_free_all (headerlist);
    }
    
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"username",     required_argument, 0, 'u'},
            {"password",  required_argument, 0, 'p'},
            {"save",  required_argument, 0, 's'},
            {"rm",  required_argument, 0, 'r'},
            {"mkdir",  required_argument, 0, 'm'},
            {"help",  no_argument, 0, 'h'},
            {"destination", required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "vbu:p:d:m:r:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            printhelp(argv[0]);
            exit(1);
            break;
        case 'u':
            strncpy(username, optarg, sizeof(username)-1);
            break;

        case 'd':
            strncpy(destination, optarg, sizeof(destination)-1);
            break;
            
        case 'p':
            strncpy(password, optarg, sizeof(password)-1);
            break;
            
        case 'm':
            assure_params();
            printf("Creating directory: %s\n", optarg);
            make_simple_request("PUT", "directory", optarg);
            break;

        case 'r':
        {
            assure_params();
            printf("Removing: %s\n", optarg);
            make_simple_request("DELETE", "remove", optarg);
            break;
        }
        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            ;
        }
    }

    assure_params();
    
    CURL *curl = NULL;
    CURLcode res;

    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headerlist=NULL;
    static const char buf[] = "Expect:";



    if (optind < argc)
    {
        int i = 0;
        while (optind < argc) {
            printf ("Uploading: %s\n", argv[optind]);
            char fieldname[64];
            snprintf(fieldname, sizeof(fieldname), "file%i", i++);
            curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, fieldname,
                 CURLFORM_FILE, argv[optind++],
                 CURLFORM_END);
        }
    }

    curl = curl_easy_init();

    headerlist = curl_slist_append(headerlist, buf);
    if(curl) {
        char url[4096+10];
        snprintf(url, sizeof(url), "%s%s", (strncmp(destination, "https://", 8) == -1 && strncmp(destination, "http://", 7) == -1)?"https://":"", destination);
        
        curl_easy_setopt(curl, CURLOPT_URL,url);

        curl_easy_setopt(curl, CURLOPT_USERNAME, username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

        struct myprogress prog = {0, curl};
        
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "Error: %s\n",
                    curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        curl_formfree(formpost);
        curl_slist_free_all (headerlist);
    }
    return 0;
}

