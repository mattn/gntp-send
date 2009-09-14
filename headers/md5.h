#ifndef uint8
#define uint8 unsigned char
#endif
 
#ifndef uint32
#define uint32 unsigned long int
#endif
 
#ifndef uint64
# ifdef _WIN32
# ifdef __GNUC__
# define uint64 unsigned __int64
# else
# define uint64 unsigned _int64
# endif
# else
# define uint64 unsigned long long
# endif
#endif
 
#ifndef byte
# define byte unsigned char
#endif
 
typedef struct {
uint32 total[2];
uint32 state[4];
uint8 buffer[64];
} md5_context;
 
void md5_starts(md5_context *ctx);
void md5_update(md5_context *ctx, const uint8 *input, uint32 length);
void md5_finish(md5_context *ctx, uint8 digest[16]);
