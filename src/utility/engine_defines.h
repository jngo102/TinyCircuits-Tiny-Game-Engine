#ifndef ENGINE_DEFINES_H
#define ENGINE_DEFINES_H

#if defined(__unix__)
    #define ENGINE_FAST_FUNCTION(x) x
#elif defined(__arm__)
    #define ENGINE_FAST_FUNCTION(x) __time_critical_func(x) // https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#ga27ef91000958320e25ff481d16786ebf
#else
    #warning "Unknown platform for defining fast functions"
    #define ENGINE_FAST_FUNCTION(x) x
#endif

#endif

#define ADC_REF_VOLTAGE 3.3f
#define ADC_RESOLUTION (1 << 12)
#define ADC_CONV_FACTOR ADC_REF_VOLTAGE / (ADC_RESOLUTION - 1)