/***********************************************************************
 *
 *	Copyright 1996, University Corporation for Atmospheric Research.
 *
 *	log.c
 *
 *	Callbacks and utilities for displaying program logging information.
 *
 *	History:
 *
 *	11/96	COMET	Original copy
 *	 7/97	COMET	Added X and Xt error handlers.
 * 	 1/04	Unidata/Chiz	Updated for stdarg
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "gui.h"
#include "genglobs.h"
#include "winpref.h"
#include <unistd.h>

#define printf gprint

int x_error();
void xt_error();
void wprint(char *fmt, va_list args);
void UnManageCB();
void PopDownCB ( Widget, XtPointer, XtPointer );
void PopUpCB ( Widget, XtPointer, XtPointer );


static Widget		text_w;	/* Scrolled text window */
Widget	This_dialog;


void
ManageWidgetCB ( Widget w, XtPointer client_data, XtPointer call_data )
{
	Widget	thisWidget;

	XmAnyCallbackStruct *cbs =
			(XmAnyCallbackStruct *) call_data;

	thisWidget = ( Widget ) client_data;

	XtManageChild ( This_dialog );
}


void
PopUpCB ( Widget w, XtPointer client_data, XtPointer call_data )
{
	Widget	shell;

	XmPushButtonCallbackStruct *cbs =
			(XmPushButtonCallbackStruct *) call_data;

/*
 *	Shell widget is passed through as client data.
 */
	shell = ( Widget ) client_data;

/*
 *	Manage the shell for this dialog.
 */
/*	XtPopup ( shell, XtGrabNone ); */
/*	XtPopup ( XtParent (This_dialog), XtGrabNone ); */
	XtManageChild ( This_dialog );
}



void CreateErrorWindowShell ( Widget parent, XtAppContext app )
{
	Widget		shell, dialog, b, rowcol;
	Arg		al[64];
	register int	ac;

/*
 *	Dialog.
 */
	shell = XtVaCreatePopupShell( "Garp Log",
			xmDialogShellWidgetClass,
			parent,
			XmNdialogType, XmDIALOG_TEMPLATE,
			XmNautoUnmanage, True,
			NULL);
/*
 *	Dialog.
 */
	ac = 0;
	XtSetArg(al[ac], XmNautoUnmanage, True ); ac++;
	XtSetArg(al[ac], XmNdialogType, XmDIALOG_TEMPLATE); ac++;
	dialog = XmCreateMessageBox ( shell,
			"dialog", al, ac );
	This_dialog = dialog;
/*
 *	Close button.
 */
	b = XtVaCreateManagedWidget( "Close",
			xmPushButtonWidgetClass, dialog,
			NULL);
	XtAddCallback ( b, XmNactivateCallback, PopDownCB, shell );

/*
 *	Rowcolumn to contain scrolled text window.
 */
	rowcol = XtVaCreateWidget("rowcol",
			xmRowColumnWidgetClass, dialog,
			NULL);
/*
 *	Scrolled text window.
 */
	XtSetArg(al[0],	XmNrows,		6);
	XtSetArg(al[1],	XmNcolumns,		80);
	XtSetArg(al[2],	XmNeditable,		False);
	XtSetArg(al[3],	XmNeditMode,		XmMULTI_LINE_EDIT);
	XtSetArg(al[4],	XmNwordWrap,		True);
	XtSetArg(al[5],	XmNscrollHorizontal,	False);
	XtSetArg(al[6],	XmNcursorPositionVisible, False);
	text_w = XmCreateScrolledText(rowcol, "text_w", al, 7);
	XtManageChild(text_w);

/*
 *	Capture Xt and Xlib errors.
 */
	XtAppSetErrorHandler(app, xt_error);
	XtAppSetWarningHandler(app, xt_error);
	(void) XSetErrorHandler ( x_error );

/*
 *	Manage.
 */
	XtManageChild ( rowcol );
	XtManageChild ( dialog );
}


void
wprint ( char *fmt, va_list args)
{
/*
 * This function prints a variable length argument list "printf"
 * style error statement to a scrolled text window.
 */
	GuiWinPrefDialogType	*wpi;
	Widget			log_text;
	char			msgbuf[BUFSIZ]; /* not getting huge strings */
	static XmTextPosition	wpr_position; /* maintain text position */
	static int		text_size;

	wpi = GetGuiWinPrefDialog();
	log_text = GetLogInfoW ( wpi );


/*
 *	System V has vsprintf, as does SUNOS. But older BSD machines 
 *	typically use _doprnt() .
 */

#ifndef NO_VPRINTF
	(void) vsprintf ( msgbuf, fmt, args );
#else /* !NO_VPRINTF */
	{
	    FILE foo;
	    foo._cnt = BUFSIZ;
	    foo._base = foo._ptr = msgbuf; /* (unsigned char *) ?? */
    	    foo._flag = _IOWRT+_IOSTRG;
	    (void) _doprnt ( fmt, args, &foo );
	    *foo._ptr = '\0'; /* plant terminating null character */
	}
#endif /* NO_VPRINTF */

/*
 *	Get maximum size of text string so we can avoid overflow.
 */
	XtVaGetValues ( log_text, XmNmaxLength, &text_size, NULL );

/*
 *	Insert new text.
 */
	if ( wpr_position < 0.8 * text_size )
	    XmTextInsert ( log_text, wpr_position, msgbuf );

	else {
	    XmTextSetString ( log_text, msgbuf );
	    wpr_position = 0;
	}

/*
 *	Reposition new text so it is viewable.
 */
	wpr_position += strlen ( msgbuf );
	XtVaSetValues ( log_text, XmNcursorPosition, wpr_position, NULL );
	XmTextShowPosition ( log_text, wpr_position );

}


void
SetVerboseLevelCB (Widget w, XtPointer client_data, XtPointer xt_call_data )
{
	int	verbose;

	XmToggleButtonCallbackStruct *call_data = 
			(XmToggleButtonCallbackStruct *) xt_call_data;


	if( GetVerboseLevel() > VERBOSE_0 )
	    printf ( "SetVerboseLevelCB\n" );

	verbose = (int) client_data;

	SetVerboseLevel ( verbose );

	if( GetVerboseLevel() > VERBOSE_0 )
	   printf ( "   verbose = %d\n", verbose );
}


void
PrintLogCB (Widget w, XtPointer client_data, XtPointer xt_call_data )
{
	XmToggleButtonCallbackStruct *call_data = 
			(XmToggleButtonCallbackStruct *) xt_call_data;

	if( GetVerboseLevel() > VERBOSE_0 )
	    printf ( "PrintLogCB\n" );

}
