/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MEMBRAINATOMICS_H
#define MEMBRAINATOMICS_H

//We want to use a clever logic here to be more portable...

//GCC atomics
#define rambrain_atomic_fetch_add(a,b) __sync_fetch_and_add(a,b)
#define rambrain_atomic_fetch_sub(a,b) __sync_fetch_and_sub(a,b)
#define rambrain_atomic_add_fetch(a,b) __sync_add_and_fetch(a,b)
#define rambrain_atomic_sub_fetch(a,b) __sync_sub_and_fetch(a,b)
#define rambrain_atomic_bool_compare_and_swap(ptr,oldval,newval) __sync_bool_compare_and_swap(ptr,oldval,newval)


#endif
