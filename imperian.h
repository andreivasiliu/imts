/*
 * Copyright (c) 2004, 2005  Andrei Vasiliu
 * 
 * 
 * This file is part of MudBot.
 * 
 * MudBot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * MudBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MudBot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/* Header file specific to Imperian. */

#define IMPERIAN_H_ID "$Name$ $Id$"


/* To make life easier. */
#define BAL_NONE		0
#define BAL_HEALELIX		1
#define BAL_HERB		2
#define BAL_SALVE		3
#define BAL_SMOKE               4

#define SPEC_NORMAL		0
#define SPEC_NOTHING		1
#define SPEC_PARTDAMAGE		2
#define SPEC_TRIP		3
#define SPEC_IGNORENEXT		4
#define SPEC_DEFENCE		5
#define SPEC_INSTAKILL		6
#define SPEC_WRITHE		7
#define SPEC_ARROWS		8
#define SPEC_AEON		9
#define SPEC_SLOW	       10
#define SPEC_YOTH	       11

#define PART_NONE	       -1

#define PART_HEAD		0
#define PART_TORSO		1
#define PART_LEFTARM		2
#define PART_RIGHTARM		3
#define PART_LEFTLEG		4
#define PART_RIGHTLEG		5
#define PART_LAST		5
#define PART_ARM		6
#define PART_LEG		7
#define PART_ONEARM		8
#define PART_ONELEG		9
#define PART_ANY	       10

#define PART_HEALED		0
#define PART_CRIPPLED		1
#define PART_MANGLED		2
#define PART_MANGLED2	        3

#define WRITHE_WEBS		0
#define WRITHE_ROPES		1
#define WRITHE_IMPALE		2
#define WRITHE_ARROW		3
#define WRITHE_TRANSFIX		4
#define WRITHE_DART		5
#define WRITHE_STRANGLE 	6
#define WRITHE_BOLT		7
#define WRITHE_GRAPPLE		8
#define WRITHE_NET		9
#define WRITHE_LAST	       10



struct trigger_table_type
{
   char *message;	/* The message to be checked against */
   int special;		/* It isn't a normal affliction trigger? */
   
   /* Special = 0, normal affliction trigger. */
   int affliction;	/* What affliction? */
   int gives;		/* Does it give (0) or does it cure (1) it? */
   
   /* Special = 2, limb damage. */
   int p_part;
   int p_level;
   
   /* Special = 5, defence. */
   char *defence;	/* What defence will this trigger change? */
   
   /* Special = 7, writhe. */
   int writhe;		/* What should we writhe from? */
   
   /* Extended options. */
   char *action;	/* Want to send something? */
   int missing_chars;	/* How many characters don't have to match? */
   int level;		/* Trigger level. Kinda complicated. */
   int silent;		/* Should we send anything to the client? */
   int nocolors;	/* Some triggers have colors in them. */
   int first_line;	/* First line, second line, or doesn't matter? */
};


struct affliction_table_type
{
   char *name;
   
   int herbs_cure;
   int smoke_cure;
   int salve_cure;
   
   int focus;
   int purge_blood;
   
   int special;
   
   int afflicted;
   int tried;

   char *diagmsg;
};

struct cure_table_type
{
   char *name;
   char *action;
   char *fullname;
   
   int balance_type;
   int outr;
};


// losing the bond with the animal spirits.
// You feel the barrier that separated you from your animal consciousness has 
// dissipated.
// pierced with 1 barbed arrows.
// 
// understanding the occult mysteries and nature of the universe as seen by 
// 
// Dagnimaer.
// 
// 
// Dagnimaer places both hands on either side of your head and stares straight 
// into your eyes. A ghostly ocher light gleams from his pupils and shoots into 
// your eyes, and your mind expands with alien and strange visions. (hallucinations)
// You yell, "No, it cannot be true! I cannot accept it! I beg of you to make it 
// stop!"
// 
// wounded with a incendiary arrow!
// 
// stained by evil.
// 


struct defences_table_type
{
   char *name;		/* A short one-word name of the defence. */
   char *action;	/* If not null, then we'll send this to the server. */
   
   int on;		/* Is it currently enabled? */
   int tried;		/* If not, and keepon is enabled, then did we already do something? */
   int nodelay;		/* Maybe we shouldn't set a timer on this one? */
   
   int keepon;		/* Should we try to keep it on? */
   int needy;		/* Does it need balance and equilibrium? */
   int standing;	/* Requires us not to be so lazy? */
   
   int needherb;	/* Needs herb balance? */
   int takeherb;	/* Uses herb balance? */
   int needsalve;	/* Needs salve balance? */
   int takesalve;	/* ...? */
   int needsmoke;	/* ...? */
   int takesmoke;	/* ...? */
   
   int takebalance;	/* Does it take balance or equilibrium? */
   
   int level;		/* Trigger level. */
};


