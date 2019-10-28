/*******************************************************************************
File: autocomplete.c
Author: Christos Garis
*******************************************************************************/

#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#define N 5                                        // number of word suggestions


/* Structures */

typedef struct Trie_Node* Trie;
typedef struct Leaf_Node* Leaf;

typedef struct Trie_Node{
  int letter;                       // -1->root node, 0->a node, ..., 25->z node
  Trie parent;                                    // Pointer to parent Trie node
  Trie next_letter[26];                         // 0->a, 1->b, ..., 24->y, 25->z
  Leaf word;                                   // Indicates if the end of a word
  int N_frequencies[9];                // Frequencies of N most used words below
  Leaf N_most_shown[9];                   // Pointers to N most used words below
}Trie_Node;

typedef struct Leaf_Node{
  int times_shown;                                    // Times the word is shown
  Trie last_letter;                        // Pointer to last letter of the word
}Leaf_Node;


/* Trie functions */

void CreateTrie(Trie* trie)
{
  int i;

  (*trie)=(Trie)malloc(sizeof(Trie_Node));       // Allocate memory for the root

  (*trie)->letter=-1;                     // The root does not indicate a letter
  (*trie)->parent=NULL;                         // The root does not have parent
  for(i=0;i<26;i++)
    (*trie)->next_letter[i]=NULL;                    // Setting pointers to NULL
  (*trie)->word=NULL;                                 // Setting pointer to NULL
  for(i=0;i<9;i++)
  {
    (*trie)->N_frequencies[i]=-1;                   // Setting frequencies to -1
    (*trie)->N_most_shown[i]=NULL;                   // Setting pointers to NULL
  }
}


void DeleteTrie(Trie* trie)
{
  int i;

  if(*trie==NULL)
    return;

  /* First delete subtrees */
  for(i=0;i<26;i++)
    DeleteTrie(&((*trie)->next_letter[i]));

  /* Then delete the word and the node */
  free((*trie)->word);
  free(*trie);
}


void InsertTrie(Trie* trie, char* w)
{
  int i,j;
  int l;                                             // l = the length of word w
  int* path;                // path = the string w not in letters but in numbers
  Trie ptr = *trie;                // pointer for moving into the trie structure
  Trie temp = NULL;     // pointer to allocate new nodes into the trie structure
  Leaf word = NULL;        // when the path is ready, pointer to end of the path

  /* Create the path */
  l=strlen(w);
  path=(int*)malloc(l*sizeof(int));
  for(i=0;i<l;i++)
    path[i]=w[i]-'a';                           // 0->a, 1->b, ..., 24->y, 25->z

  /* Insert the path for the word in the trie */
  for(i=0;i<l;i++)
  {
    if(ptr->next_letter[path[i]]==NULL)
    {
      /* We have to create a new node */
      temp=(Trie)malloc(sizeof(Trie_Node));      // Allocate memory for the temp
      ptr->next_letter[path[i]]=temp; // Assign the next_letter[path[i]] pointer

      /* We have to initialize the new node */
      temp->letter=path[i];           // The new node represents letter: path[i]
      temp->parent=ptr;                                    // Assign parent node
      for(j=0;j<26;j++)
        temp->next_letter[j]=NULL;                   // Setting pointers to NULL
      temp->word=NULL;                                // Setting pointer to NULL
      for(j=0;j<9;j++)
      {
        temp->N_frequencies[j]=-1;                  // Setting frequencies to -1
        temp->N_most_shown[j]=NULL;                  // Setting pointers to NULL
      }

      /* Move ptr */
      ptr=temp;                                          // Move to the new node
      temp=NULL;                                   // We don't need temp anymore
    }
    else
      ptr=ptr->next_letter[path[i]];           // Just move to node if it exists
  }

  /* Pointer ptr reached end of path, now insert a leaf for the word */
  word=(Leaf)malloc(sizeof(Leaf_Node));          // Allocate memory for the leaf
  word->times_shown=-1;                                      // Initialize value
  word->last_letter=ptr;                              // The leaf comes from ptr
  ptr->word=word;                                      // ptr points to the leaf
}


int ChangeFrequencies(Trie* trie, char* w, int n)
{
  int i;
  int l;                                             // l = the length of word w
  int min, min_pos;          // minimum frequency and its position in the matrix
  int exist=1;                                                        // flag #1
  int exist2=0;                                                       // flag #2
  int* path;                // path = the string w not in letters but in numbers
  Trie ptr = *trie;                // pointer for moving into the trie structure
  Leaf word;                           // pointer to the word at end of the path

  /* Create the path */
  l=strlen(w);
  path=(int*)malloc(l*sizeof(int));               // path is the word in numbers
  for(i=0;i<l;i++)
    path[i]=w[i]-'a';

  /* Move through path */
  i=0;
  while((i<l)&&(exist==1))
  {
    if(ptr->next_letter[path[i]]!=NULL)
    {
      ptr=ptr->next_letter[path[i]];           // Just move to node if it exists
      i++;
    }
    else
      exist=0;                   // If this happens, then the path doesn't exist
  }

  /* If the path doesn't exist, return 0, else do some stuff... */
  if(exist==1)
  {

    /* There must be a word connected at the end of the path, else return 0 */
    if(ptr->word!=NULL)
    {
      word=ptr->word;                                        // Fix pointer word
      word->times_shown++;                                // Times word shown ++

      /* Must fix the subtree from the ptr to the root */
      do
      {
        /* First check if node already points at word as N most shown */
        exist2=0;                       // Assume the node doesn't point at word
        i=0;                                                         // Iterator
        while((exist2==0)&&(i<n))
        {
          if(ptr->N_most_shown[i]==word)       // If the node does point at word
          {
            exist2=1;                                            // Fix the flag
            ptr->N_frequencies[i]++;                        // Fix the frequency
          }
          else
            i++;
        }

        /* If node doesn't point at word, we make it happen, by replacing the 
           less shown word */
        if(exist2==0)
        {
          min=1000;
          min_pos=1000;
          for(i=(n-1);i>-1;i--)
          {
            if(ptr->N_frequencies[i]<=min)
            {
              min=ptr->N_frequencies[i];                             // Min freq
              min_pos=i;                                    // Min freq position
            }
          }

          /* Found the less used, now check if more or less than our string w */
          if(word->times_shown>min)                     // If this, then replace
          {
            ptr->N_frequencies[min_pos]=word->times_shown;
            ptr->N_most_shown[min_pos]=word;
          }
        }

        ptr=ptr->parent;                                  // Move pointer ptr up

      }while(ptr!=*trie);

      /* Return how many times w was used since start of program's execution */
      return word->times_shown;
    }
    else
      return 0;                           // Not a word connected to end of path

  }
  else
    return 0;                                                   // Path not full
}


void SearchN(Trie trie, char* w, int n, char* line)
{
  int i,j,k,l,l1,l2;
  int* path;                // path = the string w not in letters but in numbers
  int exist=1;                                                           // flag
  char* words[n];                             // top N words to be given to user
  int buffer[50];
  int buffer2[50];
  char buffer3[50];
  int choice;                                            // choice = {1,2,...,n}
  int choice2;
  Trie ptr = trie;                 // pointer for moving into the trie structure
  Leaf word = NULL;                    // pointer to the word at end of the path
  Trie ptr2 = NULL;                // pointer for moving from ptr up to the root

  /* Initializing buffers */
  for(i=0;i<50;i++)
  {
    buffer[i]=-1;
    buffer2[i]=-1;
    buffer3[i]='\0';
  }

  /* Allocate buffer for N words */
  for(i=0;i<n;i++)
  {
    words[i]=(char*)malloc(50*sizeof(char));
    for(j=0;j<50;j++)
      words[i][j]='\0';
  }

  /* Create the path */
  l=strlen(w);
  path=(int*)malloc(l*sizeof(int));               // path is the word in numbers
  for(i=0;i<l;i++)
  {
    if(w[i]-'a'>=0)                                           // If small letter
      path[i]=w[i]-'a';
    else                                                    // If capital letter
      path[i]=w[i]-'A';
  }

  /* Move through path */
  i=0;
  while((i<l)&&(exist==1))
  {
    if(ptr->next_letter[path[i]]!=NULL)
    {
      ptr=ptr->next_letter[path[i]];           // Just move to node if it exists
      i++;
    }
    else
      exist=0;
  }

  /* If the path exists,our purpose is to get words[n] ready with the N words */
  if(exist==1)
  {
    /* Do N times for N words */
    for(i=0;i<n;i++)
    {
      /* Only if the pointer exists, else less than N words from here */
      if(ptr->N_most_shown[i]!=NULL)
      {
        word=ptr->N_most_shown[i];                           // Fix pointer word
        ptr2=word->last_letter;                              // Fix pointer ptr2

        /* Fill the buffer with the path to the root */
        j=0;
        buffer[j]=ptr2->letter;
        while(buffer[j]!=-1)
        {
          ptr2=ptr2->parent;          
          j++;
          buffer[j]=ptr2->letter;
        }

        /* Fill the buffer2 with the buffer reversed */
        for(k=0;k<j;k++)
          buffer2[k]=buffer[j-k-1];

        /* Clear buffer3 */
        for(k=0;k<50;k++)
          buffer3[k]='\0';

        /* Change the numbers to letters */
        for(k=0;k<j;k++)
          buffer3[k]=buffer2[k]+'a';             // The string is now in buffer3

        /* Copy the string to words[i] */
        strcpy(words[i],buffer3);
      }
    }

    /* Arrange words[i] alphabetically */
    /*http://www.c4learn.com/c-programs/c-program-to-sort-set-of-strings-in-alphabetical-order-using-strcmp.html*/
    for(i=1;i<n;i++)
    {
      for(j=1;j<n;j++)
      {
        if(strcmp(words[j-1],words[j])>0)
        {
          strcpy(buffer3,words[j-1]);
          strcpy(words[j-1],words[j]);
          strcpy(words[j],buffer3);
        }
      }
    }

    /* Move left if less than n words that match */
    k=0;
    for(i=0;i<n;i++)
    {
      if(strlen(words[i])==0)
        k++;                                                // k empty positions
    }
    for(i=k;i<n;i++)
      strcpy(words[i-k],words[i]);                           // Words moved left
    for(i=n-1;i>=n-k;i--)
      strcpy(words[i],"\0");      // Delete words from right, we don't need them

    /* If one matching word, return it */
    if(strlen(words[1])==0)
    {
      l = strlen(line);
      l1 = strlen(w);
      l2 = strlen(words[0]);
      strcpy(w,words[0]);                                     // Fix word buffer

      for(i=0;i<l2;i++)                                       // Fix line buffer
        line[i+l]=words[0][i+l1];
    }
    else
    {
      /* Print the matching words */
      for(i=0;i<n;i++)
      {
        if(strlen(words[i])>0)
          fprintf(stderr, "%d.%s ",i+1,words[i]);
      }

      /* Read choice */
      choice2 = getchar_silent();
      choice = choice2-'0';
      if((choice<1)||(choice>n))                                    // If letter
      {
        l = strlen(line);
        l1 = strlen(w);
        line[l] = choice2;                                    // Fix line buffer
        w[l1] = choice2;                                      // Fix word buffer
      }
      else                                                       // If {1,...,9}
      {
        l = strlen(line);
        l1 = strlen(w);
        l2 = strlen(words[choice-1]);
        strcpy(w,words[choice-1]);                            // Fix word buffer

        for(i=0;i<l2;i++)
          line[i+l]=words[choice-1][i+l1];                    // Fix line buffer
      }
    }

  }
  else  
    printf("Nothing Found.");
}


/* Other functions */

/* Ignore ' ' and '\n' */
/* From an older assignment for the Operation Systems course */
void ignore_ch(FILE* file)
{
   int ch;
   while( (ch=getc(file)) == ' ' || ch == '\n' )
   {
     // do nothing
   }
   ungetc(ch,file);
}


int getchar_silent()
{
  int ch;
  struct termios oldt, newt;

  /* Retrieve old terminal settings */
  tcgetattr(STDIN_FILENO, &oldt);

  /* Disable canonical input mode, and input character echoing. */
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );

  /* Set new terminal settings */
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  /* Read next character, and then switch to old terminal settings. */
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return ch;
}


int main(int argc, char** argv)
{
  Trie tr;                                                 // The Trie structure
  FILE* fp;                                  // File pointer for dictionary file
  FILE* fp2;                                     // File pointer for output file
  char buffer[50];                                         // Buffer for strings
  char line[80];                                              // Buffer for line
  int next;                                             // Characters from input
  int i,j,k;                                                        // Iterators


  /* Open dictionary file for read and write */
  if(argc<2)            // If no argument for dictionary given, open the default
  {
    if((fp=fopen("dictionary.txt","a+"))==NULL)
    {
      printf("Error opening $HOME/.dict\n");
      return -1;
    }
  }
  else                              // If argument for dictionary given, open it
  {
    if((fp=fopen(argv[1],"a+"))==NULL)
    {
      printf("Error opening %s\n",argv[1]);
      return -1;
    }
  }


  /* Open output file for write */
  if(argc<3)                 // If no argument for output given, open output.txt
  {
    if((fp2=fopen("output.txt","a+"))==NULL)
    {
      printf("Error opening output.txt\n");
      return -1;
    }
  }
  else                                  // If argument for output given, open it
  {
    if((fp2=fopen(argv[2],"a+"))==NULL)
    {
      printf("Error opening %s\n",argv[2]);
      return -1;
    }
  }


  /* Create the Trie tr */
  CreateTrie(&tr);


  /* Read all the strings from the open file and insert them into the Trie */
  do
  {
    fscanf(fp,"%s",buffer);
    InsertTrie(&tr,buffer);
    k=ChangeFrequencies(&tr,buffer,N);
    ignore_ch(fp);
  }while(!feof(fp));
  printf("\n");


  /* Clear the buffer */
  for(i=0;i<50;i++)
    buffer[i]='\0';
  /* Clear the line */
  for(i=0;i<80;i++)
    line[i]='\0';


  /* Read characters from the input, until Ctrl+D is given */
  i=0;
  j=0;
  while((next = getchar_silent()) != VEOF)
  {

    /* For normal characters */
    if(isalpha(next))
    {
      putchar(next);                                 // Show character on screen
      buffer[i]=next;                                   // Assign to word buffer
      line[j]=next;                                     // Assign to line buffer
      i++;                                                         // Increase i
      j++;                                                         // Increase j
    }

    /* For punctuation characters and ' ' and '\n' */
    else if(ispunct(next) || next == ' ' || next == '\n')
    {
      putchar(next);                                 // Show character on screen
      line[j]=next;                                     // Assign to line buffer
      i=0;                                    // i=0 for the next string to come
      j++;                                                         // Increase j

      if(buffer[0]!='\0')                         // For SPACE after punctuation 
      {
        if(buffer[0]-'a'>=0)                    // For small first letter string
        {
         /* If the string isn't in dictionary, insert it and change frequency */
          if(k=ChangeFrequencies(&tr,buffer,N) == 0)
          {
            InsertTrie(&tr,buffer);                                 // Insert it
            k = ChangeFrequencies(&tr,buffer,N);            // Frequency -1 -> 0
            k = ChangeFrequencies(&tr,buffer,N);             // Frequency 0 -> 1
            buffer[strlen(buffer)]='\n';        // Change line after string in d
            fprintf(fp,"%s",buffer);                     // Write string to file
          }
        }
      }

      /* Initialize the buffer */
      for(k=0;k<50;k++)
        buffer[k]='\0';

      /* If \n, initialize the line */
      if(next=='\n')
      {
        fprintf(fp2,"%s",line);
        for(k=0;k<80;k++)
          line[k]='\0';
        j=0;
      }
    }

    /* For TAB */
    else if(next=='\t')
    {
      putchar('\n');                                    // Change line on screen
      SearchN(tr,buffer,N,line);                               // Call SearchN()
      putchar('\n');                                    // Change line on screen
      putchar('\n');                                    // Change line on screen
      i=strlen(buffer);  // If don't want string from SearchN(), continue typing
      j=strlen(line);                         // j=the length of the line buffer
      printf("%s",line);                                  // Type line to screen
    }

    if(j>=80)                            // If reached end of the line buffer...
    {
      fprintf(fp2,"%s\n",line);   // ...print the line to the output file and...
      for(k=0;k<80;k++)
        line[k]='\0';                           // ...initialize the line buffer
      j=0;                                                    // j must be 0 now
    }

  }


  /* Close the open files */
  fclose(fp);
  fclose(fp2);

  /* Delete the structure */
  DeleteTrie(&tr);

  return 0;
}
