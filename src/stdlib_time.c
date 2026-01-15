#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// Time module - essential time functions

// Now: get current Unix timestamp
long wyn_time_now() {
    return time(NULL);
}

// Now millis: get current time in milliseconds
long long wyn_time_now_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Now micros: get current time in microseconds
long long wyn_time_now_micros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Sleep: sleep for seconds
void wyn_time_sleep(int seconds) {
    sleep(seconds);
}

// Sleep millis: sleep for milliseconds
void wyn_time_sleep_millis(int millis) {
    usleep(millis * 1000);
}

// Sleep micros: sleep for microseconds
void wyn_time_sleep_micros(int micros) {
    usleep(micros);
}

// Format: format timestamp as string (simple ISO format)
char* wyn_time_format(long timestamp) {
    static char buffer[32];
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

// Parse: parse ISO format string to timestamp (simplified)
long wyn_time_parse(const char* str) {
    struct tm tm_info = {0};
    sscanf(str, "%d-%d-%d %d:%d:%d",
           &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
           &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec);
    tm_info.tm_year -= 1900;
    tm_info.tm_mon -= 1;
    return mktime(&tm_info);
}

// Year: get year from timestamp
int wyn_time_year(long timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    return tm_info->tm_year + 1900;
}

// Month: get month from timestamp (1-12)
int wyn_time_month(long timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    return tm_info->tm_mon + 1;
}

// Day: get day from timestamp
int wyn_time_day(long timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    return tm_info->tm_mday;
}

// Hour: get hour from timestamp
int wyn_time_hour(long timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    return tm_info->tm_hour;
}

// Minute: get minute from timestamp
int wyn_time_minute(long timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    return tm_info->tm_min;
}

// Second: get second from timestamp
int wyn_time_second(long timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    return tm_info->tm_sec;
}
