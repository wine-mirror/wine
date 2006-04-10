/*
 *	PE dumping utility
 *
 * 	Copyright 2001 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

extern void		dump_codeview(unsigned long ptr, unsigned long len);
extern void		dump_coff(unsigned long coffbase, unsigned long len, const void* sect_map);
extern void		dump_frame_pointer_omission(unsigned long base, unsigned long len);
extern void	        dump_separate_dbg(void);
extern void             pe_dump(const void*);
