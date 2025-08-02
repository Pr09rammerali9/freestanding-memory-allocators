#ifndef MEM_H                                     
#define MEM_H
                                                 
#define memcpy(dest, src, n) __builtin_memcpy(dest, src, n)
#define memmove(dest, src, n) __builtin_memmove(dest, src, n)                                    
#define memset(s, c, n) __builtin_memset(s, c, n)                                          
#define memcmp(s1, s2, n) __builtin_memcmp(s1, s2, n)                                         
#define memchr(s, c, n) __builtin_memchr(s, c, n)                                           
#define frame_addr(l) __builtin_frame_address(l)  
#define ret_addr(l) __builtin_return_address(l)
#define swp16(x) __builtim_bswap16(x)             
#define swp32(x) __builtin_bswap32(x)
#define swp64(x) __builtin_bswap64(x)   

#endif
