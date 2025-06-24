#include <stdio.h>
#include "resource.h"

UserInfo users [MAX_CLIENT]= {0}; //초기화 갈겨주면서

int find_emtpy_user_slot(){
    for (int i=0; i<MAX_CLIENT; i++){
        if (users[i].is_activated == 0) return i;
    }
    return -1;
}
