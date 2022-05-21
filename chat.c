#define FUSE_USE_VERSION 31
#define debug

#include<fuse.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stddef.h>
#include<fcntl.h>

#ifdef debug
    char* debuglog;
#endif

static const int max_name_length = 10;


/*Define the file struct and some operations*/
/*----------------------------------------- */
static struct mfile
{
    char* name;  //file name
    char* data;  //file content
    struct mfile* son; //record the file in the next level
    struct mfile* father; //record the file in the last level
    struct mfile* next; //record the file in the same level
    struct mfile* prev; //record the file in the same level
    int isfile;
    
};
static struct mfile* minichat_root;


static struct mfile* new_mfile(){
    struct mfile* res = (struct mfile*)malloc(sizeof(struct mfile));
    res -> name = NULL;
    res -> data = NULL;
    res -> son = NULL;
    res -> father = NULL;
    res -> next = NULL;
    res -> prev = NULL;
    res -> isfile = 0;
    return res;
}
static void remove_mfile(struct mfile* f){
    if(f -> prev == NULL){     //f is the head of the files in the same level
        f -> father -> son = f -> next;
        if(f -> next!=NULL) f -> next -> prev = NULL;
    }
    else{
        f -> prev -> next = f -> next;
        if(f -> next!=NULL) f -> next -> prev = f -> prev;
    }
    free(f->data);
    free(f->name);
    free(f);
}

static void insert_mfile(struct mfile* father_file, struct mfile* son_file){
    struct mfile* p = father_file -> son;
    son_file -> next = p;
    if(p!=NULL) p -> prev = son_file;
    father_file -> son = son_file;
    son_file -> father = father_file;
}

static int find_mfile(struct mfile* root, struct mfile** res, const char* path){
    #ifdef debug
        //strcat(debuglog, "start find_mfile");
    #endif
    int i = 0;
    struct mfile* now_dir = root;
    while(path[i]!='\0'){
        if(path[i]=='/'){
            i++;
            continue;
        }
        int j = 0;
        char tmp[max_name_length];
        while(path[i]!='/' && path[i]!='\0'){
            tmp[j++]=path[i++];
        }
        tmp[j]='\0';
        if(path[i] == '/') i++;
        struct mfile* p = now_dir -> son;
        int have_find = 0;
        while(p!=NULL){
            if(strcmp(p->name, tmp)==0){
                now_dir = p;
                have_find = 1;
                break;
            }
            p = p -> next;
        }

        if (!have_find) return -ENOENT;
    }
    (*res) = now_dir;
    return 0;
}



/*minichat function*/
/*-----------------*/

static void *minichat_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 0;
	return NULL;
}


static int minichat_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{   
    #ifdef debug
        //strcat(debuglog, "start getattr");
    #endif
	(void) fi;
	memset(stbuf, 0, sizeof(struct stat));

    struct mfile* pos;
    if(find_mfile(minichat_root, &pos, path) != 0) return -ENOENT;


	if (pos -> isfile == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(pos -> data) + 1;
	}
    
	return 0;
}

static int minichat_access(const char* path, int mask){
    struct mfile* pos;
    int res = find_mfile(minichat_root, &pos, path);
    if(res != 0) return res;
    return 0;
}

static int minichat_mkdir(const char* path, mode_t mode){
    //get the new directory's name and new path
    #ifdef debug
        //strcat(debuglog, "start mkdir");
    #endif
    char dir_name[max_name_length];
    int length = strlen(path);
    char* new_path = (char*)malloc(length+1);
    memset(new_path, 0, sizeof(new_path));
    new_path[0]='\0';
    int i = length, j = 0;
    while(path[i]!='/') i--;
    strncpy(new_path, path, i);
    new_path[i]='\0';

    i++;
    while(path[i]!='\0'){
        dir_name[j++]=path[i++];
    }
    dir_name[j]='\0';

    //make new directory
    struct mfile* pos;
    if(find_mfile(minichat_root, &pos, new_path) != 0) return -ENOENT;
    free(new_path);

    struct mfile* new_dir = new_mfile();
    new_dir -> name = strdup(dir_name);
    insert_mfile(pos, new_dir);

    return 0;
}

static int minichat_rmdir(const char* path){
    struct mfile* pos;
    if(find_mfile(minichat_root, &pos, path) != 0) return -ENOENT;
    if(pos -> son != NULL) return -EPERM;
    remove_mfile(pos);
    return 0;
}

static int minichat_mknod(const char* path, mode_t mode, dev_t rdev){
    //adding a new file is the same as adding a new directory
    int res = minichat_mkdir(path, mode);
    if(res != 0) return res;

    struct mfile* pos;
    res = find_mfile(minichat_root, &pos, path);
    if(res!=0) return res;
    pos -> data = strdup("|--This is minichat room, talk freely!--|\n");
    pos -> isfile = 1;

    return 0;
}

static int minichat_open(const char* path, struct fuse_file_info* fi){
    struct mfile* pos;
    int res = find_mfile(minichat_root, &pos, path);
    if(res != 0) return res;
    return 0;
}

static int minichat_release(const char* path, struct fuse_file_info* fi){
    return 0;
}

static int minichat_unlink(const char* path){
    struct mfile* pos;
    int res = find_mfile(minichat_root, &pos, path);
    if(res != 0) return res;

    remove_mfile(pos);
    return 0;
}

static int minichat_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    //get the name of the sender and receiver
    char rec_name[max_name_length], sen_name[max_name_length];
    int length = strlen(path);
    char* rec_path = (char*)malloc(length+1);
    memset(rec_path, 0, sizeof(rec_path));
    rec_path[0]='\0';
    int i = length, j=0;
    while(path[i]!='/') i--;
    i--;
    while(path[i]!='/') i--;
    strncpy(rec_path, path, i);
    rec_path[i]='\0';

    i++;
    while(path[i]!='/'){
        sen_name[j++] = path[i++];
    }
    sen_name[j]='\0';
    i++, j=0;
    while(path[i]!='\0'){
        rec_name[j++] = path[i++];
    }
    rec_name[j]='\0';

    //write to sender
    struct mfile* pos;
    int res = find_mfile(minichat_root, &pos, path);
    if(res!=0) return res;

    char sen_name_f[14];  //max_name_length + 3 = 14
    sen_name_f[0] = '[';
    sen_name_f[1] = '\0';
    strcat(sen_name_f, sen_name);
    strcat(sen_name_f, "]\n");

    int final_length = strlen(pos -> data) + strlen(sen_name_f) + size; //number of size contains the '\0'
    char* new_data = (char*)malloc(final_length);
    memcpy(new_data, pos -> data, strlen(pos -> data)+1);
    strcat(new_data, sen_name_f);
    strncat(new_data, buf, size);
    free(pos -> data);
    #ifdef debug
        strcat(debuglog, new_data);
    #endif
    pos -> data = new_data;

    //write to receiver
    strcat(rec_path, "/");
    strcat(rec_path, rec_name);
    strcat(rec_path, "/");
    strcat(rec_path, sen_name);
    res = find_mfile(minichat_root, &pos, rec_path);
    if(res != 0){
        minichat_mknod(rec_path, 0, 0);
        res = find_mfile(minichat_root, &pos, rec_path);
    }
    pos -> data = strdup(new_data);
    
    return size;
}

static int minichat_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    struct mfile* pos;
    int res = find_mfile(minichat_root, &pos, path);
    if(res != 0) return res;

    size_t length = strlen(pos -> data) + 1;
    if(offset > length) return 0;

    size_t avail = length - offset;
    size_t rsize = (size < avail) ? size : avail;
    memcpy(buf, pos -> data + offset, rsize);

    return rsize;
}

static int minichat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags){
    struct mfile* pos;
    int res = find_mfile(minichat_root, &pos, path);
    if(res != 0) return res;

    filler(buf, ".", NULL, 0, 0);
    if(strcmp(path, "/") != 0){
        filler(buf, "..", NULL, 0, 0);
    }

    struct mfile* p = pos -> son;
    while(p != NULL){
        filler(buf, p -> name, NULL, 0, 0);
        p = p -> next;
    }
    return 0;
} 

static int minichat_utimens(const char * path, const struct timespec tv[2], struct fuse_file_info *fi){
    return 0;
}



static const struct fuse_operations minichat_oper = {
    .init      =    minichat_init,
    .getattr   =    minichat_getattr,
    .access    =    minichat_access,
    .mkdir     =    minichat_mkdir,
    .rmdir     =    minichat_rmdir,
    .mknod     =    minichat_mknod,
    .open      =    minichat_open,
    .release   =    minichat_release,
    .unlink    =    minichat_unlink,
    .write     =    minichat_write,
    .read      =    minichat_read,
    .readdir   =    minichat_readdir,
    .utimens    =   minichat_utimens,

};

int main(int argc, char* argv[]){
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    minichat_root = new_mfile();

    #ifdef debug
    debuglog = (char*)malloc(2000);
    struct mfile* log = new_mfile();
    log -> name = strdup("log");
    log -> data = debuglog;
    log -> isfile = 1;
    insert_mfile(minichat_root, log);
    #endif

    ret = fuse_main(args.argc, args.argv, &minichat_oper, NULL);
    fuse_opt_free_args(&args);
    free(minichat_root);
    return ret;
}