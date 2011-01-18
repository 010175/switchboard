/*
 *  curses_utils.cpp
 *  Switchboard
 *
 *  Created by guillaume on 08/04/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "curses_utils.h"


void curses_init(){
	initscr();			/* Start curses mode 		*/
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, FALSE);		/* Don't get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */	
	scrollok(stdscr, TRUE); /* scroll enable */
	
	log_window = newwin(LINES-1, COLS, 0, 1);
	box(log_window, 0,0);
	wrefresh(log_window);
	
}

void curses_print(int _col, int _row, string _message, int _attribut){
	
	
	move(_row,_col);
	attron(_attribut);
	printw(_message.c_str());
	attroff(_attribut);
	refresh();			/* Print it on to the real screen */
	
}

void curses_print(int _col, int _row, string _message){
	
	move(_row,_col);
	
	printw(_message.c_str());
	
	refresh();			/* Print it on to the real screen */
	
}

void curses_print(string _message){
	
	/*int _col;
	int _row;
	
	getyx(stdscr,_row,_col) ;
	move(_row+1,0);
	*/
	wscrl(stdscr, -1);
	move(0, 0);
	printw(_message.c_str());
	
	refresh();			/* Print it on to the real screen */
	
}

void curses_print(string _message, int _attribut){
	
	/*
	 
	 int _col;
	int _row;
	
	getyx(stdscr,_row,_col) ;
	
	move(_row+1,0);
	 */
	//scroll(stdscr);
	wscrl(stdscr, -1);
	move(0, 0);
	attron(_attribut);
	printw(_message.c_str());
	attroff(_attribut);
	refresh();			/* Print it on to the real screen */
	
}