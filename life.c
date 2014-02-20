#define INCL_PM            /*For † bruke vindu-systemet*/
#define INCL_DOSPROCESS    /*For † bruke tr†der*/
#define INCL_DOSSEMAPHORES /*For † bruke semaforer*/
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*   
The classic game of life has one cell type only.  A cell will live if it
has 2 or 3 neighbours (in 8 directions) otherwise it dies.
A new cell is born in an empty position, if that position
has exactly 3 neighbour cells.

This program use several cell types.  All of them behave like the classic
game of life when alone, but they are different in how they react
to other species.

Cell types:
Type a: intolerant.
Like the classic game of life, but can't live with neighbour cells of other species.
New cells won't be born into positions where other species are neighbours.
These colonies aren't very competitive, and tend to disappear after a while.

Type b: tolerant.
This type tolerate other species, as long as the total number of neighbours don't
get too high.  New cells are born into empty positions which have 3 neighbours
of this species, and no others.
Colonies of such cells may form borders with other colonies, although
they won't infiltrate them.

Type c: symbiotic I
Like type b, except that a new cell will be born even if only 2 of the
neighbours are of the same species.  The third cell can be any type.
Colonies of these types can grow into other colonies.

Type d: symbiotic II
Like type c, except that cells of this type can live with up to 4
neighbours, as long as only 3 of them are of its own species.
These cells prefer neighbours of other kinds, and will
therefore infiltrate colonies of other cells, possibly
suffocating them.  Two different species of this type
may form a symbiotic colony which will spread more
aggressively than colonies of other types.


The program start 12 cell colonies, each with a different species.
Three species will be of type a, three of type b, and so on.
The smallest possible game is 6 by 5, 30 by 25 is the recommended minimum.
Two different species of type a will be intolerant against each other.  Each
species has a distinct color.

*/

char *newcell, *ncell=0;
char *oldcell, *ocell=0;
int xs, ys, xm, s;
TID thread_id=0;
int offs[8];

BITMAPINFO2 *bm_info=0; /* Used for displaying an array as bitmap */
POINTL pkt[4];             /* necessary for GpiDrawBits */
HMTX mutex=0;

void _System threadfunc(ULONG hwnd)
{
   int x, y, o, tot[13], diff;
   char *cell;

   diff = newcell - oldcell;
   for (; ; ) {
      for (y=0; y<ys; y++) {
         cell = oldcell+y*xm;
         for (x=0; x<xs; x++,cell++) {
            memset(tot, 0, sizeof(tot));
            for (o=0; o<8; o++) tot[*(cell+offs[o])]++;
            switch ((*cell+2)/3) {
            case 0: /* empty space */
               if (tot[0]==5) {
                  for (o=1; o<13; o++) {
                     if (tot[o]==3 || (tot[o]==2 && o>=7)) {
                        *(cell+diff)=o;
                        break;
                     }
                  } /* endfor */
               }
               break;
            case 1: /* intolerant cell */
               if (tot[0]+tot[*cell] !=8 || tot[0]>6 || tot[0] < 5) *(cell+diff)=0;
               break;
            case 2: /* tolerant cell */
            case 3: /* symbiotic I */
               if (tot[0]>6 || tot[0]<5) *(cell+diff)=0;
               break;
            case 4: /* symbiotic II */
               if (tot[0]>6 || tot[0]<4 || tot[*cell]==4) *(cell+diff)=0;
               break;
            }
         } /* endfor */
      } /* endfor */
      DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT);
      memcpy(ocell, ncell, xm*(ys+2));
      DosReleaseMutexSem(mutex);
      WinInvalidateRect(hwnd,0,TRUE);
      DosSetPriority(2, PRTYC_NOCHANGE, -5, 0);
   } /* endforever */
}

MRESULT EXPENTRY vindufunk(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
 HPS hps;
 RECTL rcl;
 switch(msg) {
  case WM_CLOSE:
   WinPostMsg(hwnd, WM_QUIT, 0, 0);
   break;
  case WM_PAINT: 
   hps = WinBeginPaint(hwnd,0,&rcl);
   {
    /* ocell is a "shared" resource */
    DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT);
    GpiDrawBits(hps, ocell, bm_info, 4, (PPOINTL) &pkt, ROP_SRCCOPY, BBO_IGNORE);
    DosReleaseMutexSem(mutex);
   }
   WinEndPaint(hps);
   DosSetPriority(2, PRTYC_NOCHANGE, 5, thread_id);
   break;
  case WM_COMMAND:
   switch (SHORT1FROMMP(mp1)) {
     }
   break;
  case WM_DESTROY:
   break;
  default:
   /*La default-funk ta seg av tingene*/
   return(WinDefWindowProc(hwnd, msg, mp1, mp2));
 }
 return 0;
}

void initvindu(HWND hwnd)
{
 RECTL rectl, titlerect;

 WinQueryWindowRect(WinWindowFromID(hwnd, FID_TITLEBAR), &titlerect);
 WinQueryWindowRect(HWND_DESKTOP, &rectl);
 WinSetWindowPos(hwnd, HWND_TOP, (rectl.xRight - s*xs) >> 1, (rectl.yTop - s*ys) >> 1,
        s*xs, s*ys + titlerect.yTop-titlerect.yBottom, SWP_SIZE | SWP_SHOW | SWP_MOVE);
 /* Start the computationally intensive thread *after* the window has got the correct size - 
     it looks better that way... */
 DosResumeThread(thread_id);
}

void glob_init()
{
   int x,y;
   RGB2 *f;
   FILE *fil;
  /* Coordinates for neighbourhood */
  offs[0] = -1;
  offs[1] =  1;
  offs[2] = -xm;
  offs[3] =  xm;
  offs[4] = -1-xm;
  offs[5] = -1+xm;
  offs[6] =  1-xm;
  offs[7] =  1+xm;

  /* Coordinates for GpiDrawBits */
  pkt[0].x=0;
  pkt[0].y=0;
  pkt[1].x=xs*s-1;
  pkt[1].y=ys*s-1;
  pkt[2].x=1;
  pkt[2].y=1;
  pkt[3].x=xs+1;
  pkt[3].y=ys+1;

  /* Pointers to cell arrays */
  oldcell = ocell+offs[7];
  newcell = ncell+offs[7];


  /*  BITMAPINFO2 structure */
  bm_info->cbFix=40;
  bm_info->cx=xm;
  bm_info->cy=ys+2;
  bm_info->cPlanes=1;
  bm_info->cBitCount=8;
  bm_info->ulCompression=BCA_UNCOMP;
  bm_info->cbImage=0;
  bm_info->cclrUsed=13;
  bm_info->cclrImportant=13;

  fil = fopen("life.cnf", "r");
  if (!fil) exit(-1);
  f=(RGB2 *) ((char *)bm_info + bm_info->cbFix);
  for (x=0; x<13; x++) {
     int r,g,b;
     fscanf(fil, " %i %i %i\n", &r, &g, &b);
     f->bRed=r; f->bGreen=g; f->bBlue=b;
     f->fcOptions=0;
     f++;
  } /* endfor */

  /* Initial cells, 12 rectangular colonies of different species */
  {
     int celltyp[4][3];
     for (y=0; y<3; y++) {
        for (x=0; x<4; x++) {
           fscanf(fil, " %i", &celltyp[x][2-y]);
        } /* endfor */
     } /* endfor */
     for (x=0; x<xs; x++) {
        for (y=0; y<ys; y++) {
           *(oldcell+x+xm*y) = (rand() % 3) ? 0 : celltyp[(4*x)/xs][(3*y)/ys];
        } /* endfor */
     } /* endfor */
  }
  fclose(fil);
}

#define VINDU_ID 42
main(int argc, char *argv[])
{
 HMQ hmq=0; /*Handle message queue*/
 HAB hab=0;
 HWND hwndframe=0;
 char vindutype[]="life";
 ULONG vflagg = FCF_TITLEBAR | FCF_SYSMENU | FCF_AUTOICON
	| FCF_TASKLIST | FCF_SHELLPOSITION 
	| FCF_HIDEBUTTON | FCF_MINBUTTON;
 HWND hwndclient; /*Handle frame window*/
 QMSG beskjed; /*Beskjed fra meldingsk›*/

 srand(time(0)); /* So we don't get the same game of life each time */

 do {

  if (argc != 4) break;
  xs = atoi(argv[1]);
  ys = atoi(argv[2]);
  s = atoi(argv[3]);
  xm = xs + 2;
  xm += 4 - xm % 4;
 
  ocell = (char *) calloc(1,xm * (ys + 2));
  if (!ocell) break;
  ncell = (char *) calloc(1,xm * (ys + 2));
  if (!ncell) break;
  bm_info=(PBITMAPINFO2) calloc(40+13*sizeof(RGB2),1);
  if (!bm_info) break;
  glob_init();
  if ((hab=WinInitialize(0)) == 0) break;
  if ((hmq=WinCreateMsgQueue(hab,0)) == 0) break;
  if (!WinRegisterClass(hab,vindutype,vindufunk,0,0)) break;
  hwndframe = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE | FS_BORDER, &vflagg,
	vindutype, "Life", FID_CLIENT, 0, VINDU_ID,
	&hwndclient);
  /* WS_VISIBLE is necessary, as initvindu is measuring the titlebar */
  if (!hwndframe) break;

  DosCreateMutexSem(0, &mutex, 0, 0);
  if (!mutex) break;
  if (DosCreateThread(&thread_id, threadfunc, hwndframe, 1,16384)) break;
  initvindu(hwndframe);
 

  while(WinGetMsg(hab, &beskjed,0,0,0)) WinDispatchMsg(hab, &beskjed);  
 } while (0);
 if (mutex) DosCloseMutexSem(mutex);
 if (bm_info) free(bm_info);
 if (ocell) free(ocell);
 if (ncell) free(ncell);
 if (thread_id) DosKillThread(thread_id);
 if (hwndframe) WinDestroyWindow(hwndframe);
 if (hmq) WinDestroyMsgQueue(hmq);
 if (hab) WinTerminate(hab);
}

