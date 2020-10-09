//
// Created by david on 7/10/20.
//

#ifndef REDFLAG_ERROR_H
#define REDFLAG_ERROR_H

void panic(const char* message);
#define NOT_IMPLEMENTED panic("Not implemented")
#define UNREACHABLE panic("Unreachable")


#endif //REDFLAG_ERROR_H
