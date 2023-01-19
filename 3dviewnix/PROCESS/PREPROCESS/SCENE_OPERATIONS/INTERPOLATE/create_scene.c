/*
  Copyright 1993-2008 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

 
 








/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/


#include <stdio.h>

main()
{
	unsigned char data1[7][7], data2[7][7];
	FILE *fp;



	data1[0][0] = 10;
	data1[0][1] = 10;
	data1[0][2] = 10;
	data1[0][3] = 10;
	data1[0][4] = 10;
	data1[0][5] = 10;
	data1[0][6] = 10;
	
	data1[1][0] = 10;
	data1[1][1] = 60;
	data1[1][2] = 60;
	data1[1][3] = 60;
	data1[1][4] = 60;
	data1[1][5] = 60;
	data1[1][6] = 10;
	
	data1[2][0] = 10;
	data1[2][1] = 50;
	data1[2][2] = 70;
	data1[2][3] = 80;
	data1[2][4] = 70;
	data1[2][5] = 50;
	data1[2][6] = 10;
	
	data1[3][0] = 10;
	data1[3][1] = 40;
	data1[3][2] = 90;
	data1[3][3] = 100;
	data1[3][4] = 90;
	data1[3][5] = 40;
	data1[3][6] = 10;
	
	data1[4][0] = 10;
	data1[4][1] = 60;
	data1[4][2] = 70;
	data1[4][3] = 80;
	data1[4][4] = 70;
	data1[4][5] = 60;
	data1[4][6] = 10;
	
	data1[5][0] = 10;
	data1[5][1] = 20;
	data1[5][2] = 20;
	data1[5][3] = 20;
	data1[5][4] = 20;
	data1[5][5] = 20;
	data1[5][6] = 10;
	
	data1[6][0] = 10;
	data1[6][1] = 10;
	data1[6][2] = 10;
	data1[6][3] = 10;
	data1[6][4] = 10;
	data1[6][5] = 10;
	data1[6][6] = 10;
	

	/* IMAGE 2 */
	data2[0][0] = 10;
	data2[0][1] = 10;
	data2[0][2] = 10;
	data2[0][3] = 10;
	data2[0][4] = 10;
	data2[0][5] = 10;
	data2[0][6] = 10;

	data2[1][0] = 10;
	data2[1][1] = 20;
	data2[1][2] = 20;
	data2[1][3] = 20;
	data2[1][4] = 20;
	data2[1][5] = 20;
	data2[1][6] = 10;
	
	data2[2][0] = 10;
	data2[2][1] = 60;
	data2[2][2] = 70;
	data2[2][3] = 80;
	data2[2][4] = 70;
	data2[2][5] = 60;
	data2[2][6] = 10;

	data2[3][0] = 10;
	data2[3][1] = 40;
	data2[3][2] = 90;
	data2[3][3] = 100;
	data2[3][4] = 90;
	data2[3][5] = 40;
	data2[3][6] = 10;
	
	data2[4][0] = 10;
	data2[4][1] = 50;
	data2[4][2] = 70;
	data2[4][3] = 80;
	data2[4][4] = 70;
	data2[4][5] = 50;
	data2[4][6] = 10;
	
	data2[5][0] = 10;
	data2[5][1] = 60;
	data2[5][2] = 60;
	data2[5][3] = 60;
	data2[5][4] = 60;
	data2[5][5] = 60;
	data2[5][6] = 10;
	
	data2[6][0] = 10;
	data2[6][1] = 10;
	data2[6][2] = 10;
	data2[6][3] = 10;
	data2[6][4] = 10;
	data2[6][5] = 10;
	data2[6][6] = 10;
	

	fp=fopen("ttt", "wb");


	/* Volume 1 */
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
exit(0);

	/* Volume 2 */
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);

	/* Volume 3 */
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);

	/* Volume 4 */
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);

	/* Volume 5 */
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);

	/* Volume 6 */
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);

	/* Volume 7 */
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);
	fwrite(data2, 49, 1, fp);
	fwrite(data1, 49, 1, fp);

}
