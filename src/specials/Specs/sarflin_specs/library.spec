//
// File: library.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define TOMES_FILE         "tomes/"
#define TOMES_DIR          "tomes/"
#define TOMES_DIR_DAROM    "tomes/darom/"
#define TOMES_DIR_ZHENGI   "tomes/zhengi/"
#define TOMES_DIR_SILVER   "tomes/silver/"
#define TOMES_DIR_HTOS     "tomes/htos/"
#define TOMES_DIR_NTHALOS  "tomes/new_thalos/"
#define TOMES_DIR_MONK     "tomes/monk_guild/"

#include <unistd.h>
void read_page(FILE *fh,int page,struct char_data *ch)
{
   long *list;
   int count;
   char *text_string; 

   fseek(fh,0-sizeof(count),2);
   fread(&count,sizeof(count),1,fh);
   if (page <= count-1)
   {
      list = (long *) malloc(sizeof(long)*200);
      fseek(fh,0-((count*sizeof(long)) + sizeof(count)),2);
      fread(list,sizeof(long),count,fh);

      fseek(fh,list[page],0);
      text_string = fread_string(fh,buf);
      text_string++;

      page_string(ch->desc,text_string,0);
      text_string--;

      free (text_string);
      free (list);
   }
}
SPECIAL(library)
{
  FILE *book = NULL;
  char *text_string;
  char cat_name[100], dir_name[60];
  struct obj_data *me2 = (struct obj_data *)me;

  if (ch->in_room->number == 8315)
    strcpy(dir_name, TOMES_DIR_DAROM);
  else if (ch->in_room->number == 5551)
    strcpy(dir_name, TOMES_DIR_NTHALOS);
  else if (ch->in_room->number == 21011)
    strcpy(dir_name, TOMES_DIR_MONK);
  else
    strcpy(dir_name, TOMES_DIR_DAROM);

  if (!CMD_IS("say") && !CMD_IS("'")) return 0;
  skip_spaces(&argument);
  if (!*argument)
    return 0;
  half_chop(argument,buf,buf2);

  if (!strncasecmp(buf, "catalog", 7) && GET_OBJ_VAL(me2,1))  {
    sprintf(cat_name,"%scatalog-%d",dir_name,GET_OBJ_VAL(me2,1));
    book = fopen (cat_name,"rt");
    if (book == NULL) {
      sprintf(buf,"Tome catalog not found [%s]",cat_name);
      slog (buf);
      return 1;
    }
    text_string = fread_string(book,buf);
    page_string(ch->desc,text_string,0);
    free (text_string);
    fclose(book);
    return 1;
  }
  if (!strncasecmp(buf, "help", 4) && GET_OBJ_VAL(me2,1))  {
    
    send_to_char("I am a bookstand. I respond to spoken commands\n",ch);
    send_to_char("I respond to the following commands:\n\n",ch);
    send_to_char("catalog                 -- list of books avalable.\n",ch);
    send_to_char("bring <catalog number>  -- bring a book to the stand.\n",ch);
    send_to_char("index                   -- turn the book to the index.\n",ch);
    send_to_char("read <page number>      -- turn the book to the page number you want.\n",ch);
    return 1;
  }
  
  if (!strncasecmp(buf, "help", 4) && !GET_OBJ_VAL(me2,1))  {
    send_to_char("I am a book.  I respond to spoken commands\n",ch);
    send_to_char("index                --  turn the book to the index.\n",ch);
    send_to_char("read <page number>   --  turn the book to the page number you want.\n",ch);
    return 1;
  }
  
  if (!strncasecmp(buf, "bring", 5) && GET_OBJ_VAL(me2,1))  {
    
    sprintf(cat_name,"%stome-%d.tdx",dir_name,atoi(buf2));
    if (access(cat_name,0) == 0) {
      send_to_char("I have retrived that book for you.\n",ch);
      GET_OBJ_VAL(me2,0) = atoi(buf2); 
    } else { 
      send_to_char("I am sorry that book must be on loan.\r\n"
		   "Say 'catalog' to see what we have.\r\n", ch);
    }
    return 1;
  }
  
  if (!strncasecmp(buf, "index", 5)) {
    sprintf(cat_name,"%stome-%d.tdx",dir_name,GET_OBJ_VAL(me2,0));
    if ((book = fopen (cat_name,"rt"))!= NULL)  {
      read_page(book,0,ch);
      fclose(book);
      return 1;
    } else {
      sprintf(buf,"could not open tome: [%s]",cat_name);
      slog (buf);
      return 1;
    }
  }
  if (!strncasecmp(buf, "status", 6))  {
    sprintf(buf,"Looking at book cataloged as:%d\n flags are %d %d %d %d\n",
	    GET_OBJ_VAL(me2,0),GET_OBJ_VAL(me2,0),
	    GET_OBJ_VAL(me2,1),GET_OBJ_VAL(me2,2),GET_OBJ_VAL(me2,3));
    send_to_char (buf,ch);
    return 1;
  }
  
  if (!strncasecmp(buf, "read", 4))  {
    int num;
    sprintf(cat_name,"%stome-%d.tdx",dir_name,GET_OBJ_VAL(me2,0) );
    if ((book = fopen (cat_name,"rt"))!=NULL)  {
      num = atoi(buf2);
      if (num == 0)  {
	send_to_char("I am sorry that is not a page in the book.\r\n"
		     "Type index to view the contents.\r\n",ch);  
      } else {
	
	read_page(book,num,ch);
      }
      fclose (book);
    } else  {
      sprintf(buf,"could not open tome: [%s]",cat_name);
      slog (buf);
    }
    return 1;
  }
  return 0;
}


