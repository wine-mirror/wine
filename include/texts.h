
/*
 * texts.h - String constants are read from Xresources/app-defaults
 * (c) 1994 Jochen Hein ( Hein@Student.TU-Clausthal.de )
 */

/*
 * Type-description for buttons
 */

typedef struct tButtonDesc {
  char *Label;              /* Label of the Button */
  char Hotkey;               /* Hotkey to press this Button */
} ButtonDesc;

typedef struct tButtonTexts {
  ButtonDesc Yes;
  ButtonDesc No;
  ButtonDesc Ok;
  ButtonDesc Cancel;
  ButtonDesc Abort;
  ButtonDesc Retry;
  ButtonDesc Ignore;
} ButtonTexts;


