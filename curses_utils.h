/*
 *  curses_utils.h
 *  Switchboard
 *
 *  Created by guillaume on 08/04/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <curses.h>
#include <iostream>
#include <string>



using namespace std;

WINDOW *log_window;

void curses_init();
void curses_print(string _message);
void curses_print(string _message, int _attribut);
void curses_print(int x, int y, string _message);
void curses_print(int x, int y, string _message,int _attribut);

