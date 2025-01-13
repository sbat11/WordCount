#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

// Size used for buffer when reading from file
#define BUFFER_SIZE 128

// Determines if char is a letter or apostrophe because 
// those are the valid characters used for making words 
#define isLetter(c) ((c >= 'A') && (c <= 'Z') || (c >= 'a') && (c <= 'z') || (c == '\''))
 
/*
            General Logic

// function to read command line arguments and update list accordingly
void updateWordNodeList(WordNode* head, char* argument):
    if argument is file:
        updateListFromFile(head, argument)
    else if argument is directory:
        for file in argument
            updateWordNodeList(head, argument)
    else:
        Error

// given a filename update word list
void updateListFromFile(WordNode* head, char* filename):
    for every word in filename:
        updateListFromWord() 

void updateListFromWord(WordNode* head, char* word)
    if head == null:
        head = createWordNode(word)
        return

    prev = null
    ptr = head
    found = false

    while ptr not null:
        if ptr.word == word
            ptr.count++
            found = true
        prev = ptr
        ptr = ptr.next

    if (not found)
        prev.next = createWordNode(word)
    return
    
*/

// Struct to hold words that appear and how 
// many times they have been seen
typedef struct WordNode {
    char* word;
    int count;
    struct WordNode* next;
} WordNode;

WordNode* createWordNode(char* word) {
    WordNode *node = (WordNode*)malloc(sizeof(WordNode));
    node->word = strdup(word);
    node->count = 1;
    node->next = NULL;
    return node;
}

void printList(WordNode* head) {
    WordNode* ptr = head;
    while (ptr != NULL) {
        printf("%s: %d\n", ptr->word, ptr->count);
        ptr = ptr->next;
    }
    printf("\n");
}


///////////////////// UPDATE LIST SECTION /////////////////////
// Update list given a word 
void updateListFromWord(WordNode** headPtr, char* target) {
    // Create first entry in list if empty
    if (*headPtr == NULL) {
        *headPtr = createWordNode(target);
        return;
    }

    // Traverse list and update an entry if target is found
    WordNode* prev = NULL;
    WordNode* curr = *headPtr;
    int found = 0;
    while (curr != NULL) {
        if(strcmp(curr->word, target) == 0) {
            curr->count += 1;
            found = 1;
        }
        prev = curr;
        curr = curr->next;
    }
    // If target is not found create new entry at end of list
    if (!found) {
        prev->next = createWordNode(target);
    }
}

// Update list with all words from given file
void updateListFromFile(WordNode** headPtr, char* filename) {
    // Open file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    // Read file and store bytes in buffer
    char buffer[BUFFER_SIZE];
    int bytesRead;
    
    int inWord = 0;
    int wordLength = 0;
    int wordCapacity = BUFFER_SIZE;
    char* word = (char*)malloc(wordCapacity * sizeof(char)); 
    
    do {
        bytesRead = read(fd, buffer, BUFFER_SIZE);
        for (int i = 0; i < bytesRead; i++) {
            char c = buffer[i];
            if (isLetter(c) || c == '-') { // Part of a word
                if (!inWord) {
                    inWord = 1;
                    wordLength = 0;
                }
                if (wordLength >= wordCapacity) {
                    wordCapacity *= 2;
                    word = realloc(word, wordCapacity);
                }
                word[wordLength++] = c;
            } 
            else { 
                if (inWord) {
                    word[wordLength] = '\0';
                    for(int i = 1; i < wordLength; i++){
                        if(isLetter(word[i-1]) && isLetter(word[i+1])){
                            continue;
                        }
                        else{
                            while (word[i + 1] == '-') {
                                i++; 
                            }
                            if (inWord) {
                                word[wordLength] = '\0';
                                updateListFromWord(headPtr, word);
                                inWord = 0;
                                wordLength = 0;
                            }
                        }
                    }
                    inWord = 0;
                    wordLength = 0;
                }
            }
        }
    }while(bytesRead > 0);
    if (inWord) {
        word[wordLength] = '\0';
        updateListFromWord(headPtr, word);
    }
    
    free(word);
    close(fd);
}

// Update list from directory or file names
void updateList(WordNode** headPtr, char* argument) {
    struct stat path_stat;
    if (stat(argument, &path_stat) != 0) {
        perror("stat");
        return;
    }
    // if argument is a directory call updateList on each of its entries
    // this handles directories within directories
    if (S_ISDIR(path_stat.st_mode)) {
        printf("%s is a directory.\n", argument);
        DIR* dir = opendir(argument);
        if (dir == NULL) {
            perror("opendir");
            return;
        }

        // dont include . and .. for updating list
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) { // Read each entry in the directory        
            if ((strcmp(".", entry->d_name) == 0) || (strcmp("..", entry->d_name) == 0)){
                continue;
            }
            // argument/entry->d_name\0
            char* fullPath = malloc(strlen(argument) + 1 + strlen(entry->d_name) + 1);
            strcpy(fullPath, argument);
            strcat(fullPath, "/");
            strcat(fullPath, entry->d_name);
            //printf("Subfile Name: %s\n", entry->d_name);
            //printf("Fullpath Name: %s\n\n", fullPath);
            updateList(headPtr, fullPath);
        }

    } else if (S_ISREG(path_stat.st_mode)) {
        printf("%s is a regular file.\n", argument);
        updateListFromFile(headPtr, argument);
    } else {
        printf("%s is neither a file nor a directory.\n", argument);
    }
}

// free all of the wordnodes and the words inside them
void freeList(WordNode* head) {
    WordNode* ptr = head;
    while(ptr != NULL) {
        WordNode* next = ptr->next;
        free(ptr->word);
        free(ptr);
        ptr = next;
    }
    printf("Successfully freed memory\n");
}

///////////////////// MERGE SORT SECTION /////////////////////
int compareWordNodes(WordNode *a, WordNode *b) {
    if (a->count > b->count) {
        return -1; 
    } 
    else if (a->count < b->count) {
        return 1; 
    } 
    else {
        return strcmp(a->word, b->word); // Lexicographic order if counts are the same
    }
}

WordNode* mergeSortedLinkedLists(WordNode* head1, WordNode* head2){
    if (head1 == NULL){
        return head2;
    }
    if (head2 == NULL){
        return head1;
    }

    WordNode* result = NULL;

    if (compareWordNodes(head1, head2) <= 0) {
        result = head1;
        result->next = mergeSortedLinkedLists(head1->next, head2);
    }
    else {
        result = head2;
        result->next = mergeSortedLinkedLists(head1, head2->next);
    }

    return result;
}

void splitLinkedList(WordNode* head, WordNode** front, WordNode** back) {
    WordNode* slow = head;
    WordNode* fast = head->next;

    while (fast != NULL){
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *front = head;
    *back = slow->next;
    slow->next = NULL;
}



WordNode* mergeSort(WordNode* head) {
    if (head == NULL || head->next == NULL) {
        return head;
    }

    WordNode* front;
    WordNode* back;
    splitLinkedList(head, &front, &back);

    front = mergeSort(front);
    back = mergeSort(back);

    return mergeSortedLinkedLists(front, back);
}

int main (int argc, char ** argv) {
    WordNode* list = NULL;
    for(int i = 1; i < argc; i++) {
        updateList(&list, argv[i]);
    }
    list = mergeSort(list);
    printList(list);

    freeList(list);
    
    return EXIT_SUCCESS;
}