/*
 * Keyboard definitions
 *
 */

#ifndef __WINE_KEYBOARD_H
#define __WINE_KEYBOARD_H

#define WINE_VKEY_MAPPINGS\
		vkcase('!', '1')\
		vkcase('@', '2')\
		vkcase('#', '3')\
		vkcase('$', '4')\
		vkcase('%', '5')\
		vkcase('^', '6')\
		vkcase('&', '7')\
		vkcase('*', '8')\
		vkcase('(', '9')\
		vkcase(')', '0')\
		vkcase2('`', '~', 0xc0)\
		vkcase2('-', '_', 0xbd)\
		vkcase2('=', '+', 0xbb)\
		vkcase2('[', '{', 0xdb)\
		vkcase2(']', '}', 0xdd)\
		vkcase2(';', ':', 0xba)\
		vkcase2('\'', '"', 0xde)\
		vkcase2(',', '<', 0xbc)\
		vkcase2('.', '>', 0xbe)\
		vkcase2('/', '?', 0xbf)\
		vkcase2('\\', '|', 0xdc)

#endif  /* __WINE_KEYBOARD_H */
