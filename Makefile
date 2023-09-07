#    exyflash - Exynos Flash Tool for Google Tensor and other Samsung Exynos Devices - http://github.com/CE1CECL/exyflash
#    Copyright (C) 2007-2023 Christopher Lentocha <christopherericlentocha@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published
#    by the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.

%.o: %.c exyflash.c
	$(CC) $< -o $@ -lusb-1.0 -static --static -c

exyflash: exyflash.o
	$(CC) $< -o $@ -lusb-1.0 -static --static

.PHONY: clean
clean:
	rm -rfv exyflash.o exyflash exyflash.exe
