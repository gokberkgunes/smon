#ifndef COMMON_DEFS
#define COMMON_DEFS


extern volatile sig_atomic_t rwflag;
extern long sleepamt;
extern float sec2mbps;

void setflag(int signum);
void die(const char *errmsg, ...);
long arg2pi(char *flag, char *var);
int alldiskrw(void);

#endif
