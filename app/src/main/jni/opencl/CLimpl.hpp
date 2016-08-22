//
// Created by piotr on 16/08/16.
//

#ifndef MYVRPET_CLIMPL_H
#define MYVRPET_CLIMPL_H





void dumpInfo();
extern "C" cl::Context initOpenCL();
cl::Program initProgram(const char src[]);
cl::CommandQueue initQueue();
bool openClInitialized();




#endif //MYVRPET_CLIMPL_H
