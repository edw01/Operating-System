#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LINE_SIZE 300
#define MAX_PATH_SIZE 300

// valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./tema

struct Dir;
struct File;

typedef struct Dir
{
	char *name;
	struct Dir *parent;
	struct File *head_children_files;
	struct Dir *head_children_dirs;
	struct Dir *next;
} Dir;

typedef struct File
{
	char *name;
	struct Dir *parent;
	struct File *next;
} File;

///////////////////////////// Functii ajutatoare /////////////////////////////
Dir *alloc_dir(char *name) {
	Dir *my_dir = malloc(sizeof(Dir));
	DIE(my_dir == NULL, "Eroare alocare director!\n");
	my_dir->name = malloc(sizeof(name));
	DIE(my_dir->name == NULL, "Eroare alocare nume director!\n");
	memcpy(my_dir->name, name, sizeof(name));
	my_dir->parent = NULL;
	my_dir->head_children_dirs = NULL;
	my_dir->head_children_files = NULL;
	my_dir->next = NULL;
	return my_dir;
}

File *alloc_file(Dir *parent, char *name) {
	File *my_file = malloc(sizeof(File));
	DIE(my_file == NULL, "Eroare alocare fisier!\n");
	my_file->name = malloc(sizeof(name)); 
	DIE(my_file->name == NULL, "Eroare alocare nume fisier!\n");
	memcpy(my_file->name, name, sizeof(*name));
	my_file->parent = parent;
	my_file->next = NULL;
	return my_file;
}
//////////////////////////////////////////////////////////////////////////////

void touch(Dir *parent, char *name)
{
	// Cream noul fisier
	File *new_file = alloc_file(parent, name);

	// Verificam daca in folderul parinte exista alt fisier.
	// Daca exista, atunci iteram prin lista de fisiere pana ajungem
	// la ultimul, apoi stocam fisierul nostru acolo.
	if (parent->head_children_files == NULL) {
		parent->head_children_files = new_file;
	} else {
		File* current = parent->head_children_files;
		while (current != NULL) {
			// Verificam daca exista un alt fisier cu acelasi nume
			if (!strcmp(current->name, name)) {
				free(new_file->name);
				free(new_file);
				printf("File already exists\n");
				return;
			}
			// Pentru a nu folosi inca un pointer care sa retina adresa
			// precedenta a current, am folosit aceasta conditie.
			if(current->next == NULL)
				break;
			current = current->next;
		}
		current->next = new_file;
	}
}

void mkdir(Dir *parent, char *name)
{
	Dir *new_dir = alloc_dir(name);
	new_dir->parent = parent;

	// Verificam daca in directorul parinte exista alt director.
	// Daca exista, atunci iteram prin lista de directoare pana ajungem
	// la ultimul, apoi stocam directorul nostru acolo.
	if (parent->head_children_dirs == NULL) {
		parent->head_children_dirs = new_dir;
	} else {
		Dir* current = parent->head_children_dirs;
		while (current != NULL) {
			// Verificam daca exista un alt director cu acelasi nume
			if (!strcmp(current->name, name)) {
				free(new_dir->name);
				free(new_dir);
				printf("Directory already exists\n");
				return;
			}
			// Pentru a nu folosi inca un pointer care sa retina adresa
			// precedenta a current, am folosit aceasta conditie.
			if(current->next == NULL)
				break;
			current = current->next;
		}
		current->next = new_dir;
	}
}

void ls(Dir *parent) {
	Dir *current_dir = parent->head_children_dirs;
	while (current_dir != NULL) {
		printf("%s\n", current_dir->name);
		current_dir = current_dir->next;
	}

	File *current_file = parent->head_children_files;
	while (current_file != NULL) {
		printf("%s\n", current_file->name);
		current_file = current_file->next;
	}
}

void rm(Dir *parent, char *name) {
	File* current;

	// Cazul 1 - fisierul cautat este primul din director
	if (parent->head_children_files != NULL &&
		!strcmp(parent->head_children_files->name, name)) {
		// Current este setat pe al doilea fisier din lista, fie el si null
		current = parent->head_children_files->next;
		free(parent->head_children_files->name);
		free(parent->head_children_files);
		parent->head_children_files = current;
		return;
	}

	current = parent->head_children_files;

	// Cazul 2 - fisierul este diferit de primul din director
	while (current != NULL) {
		File* prev = current;
		current = current->next;
		
		// Daca current a devenit null, deja am verificat daca
		// ultimul element din lista are numele cautat
		if (current == NULL)
			break;

		if (!strcmp(current->name, name)) {
			prev->next = current->next;
			free(current->name);
			free(current);
			return;
		}
	}

	if(current == NULL)
		printf("Could not find the file\n");
}

void __rmdir(Dir *target) {
	Dir* parent_directory = target->head_children_dirs;
	Dir* child_directory;
	File* nested_files;
	File* next_file;
	File* current_file;
	int ok = 1;

	// Pentru cazul in care in directorul nostru avem doar fisiere
	// folosim "ok" pentru a confirma ca acesta este cazul pe care mergem
	// apoi sarim la "STEP2"
	if (target->head_children_dirs == NULL) {
		if (target->head_children_files != NULL) {
			parent_directory = target;
			ok = 0;
			goto STEP2;
		}
		free(target->name);
		free(target);
		return;
	}

	// Printam numele directorului parinte
STEP1:
	// Verfificam daca directorul curent (parinte) are un alt director
	// inauntrul sau. Daca are, reluam pasul pana ajungem la ultimul director.
	child_directory = parent_directory->head_children_dirs;
	if (child_directory != NULL) {
		parent_directory = child_directory;
		goto STEP1;
	} else {
STEP2:
		// Am ajuns la ultimul director si verificam daca are fisiere
		// in el pe care sa le stergem. Dupa ce le stergem, setam head-ul NULL

		nested_files = parent_directory->head_children_files;
		if(nested_files != NULL) {
			current_file = nested_files;
			next_file = current_file->next;
			
			while (next_file != NULL) {
				free(current_file->name);
				free(current_file);
				current_file = next_file;
				next_file = next_file->next;
			}
			free(current_file->name);
			free(current_file);

			parent_directory->head_children_files = NULL;
			// Daca "ok"-ul nostru devine este 0, inseamna ca mergem pe
			// primul caz si este suficient sa ne oprim dupa ce stergem
			// fisierele din director.
			if (ok == 0) {
				free(parent_directory->name);
				free(parent_directory);
				return;
			}
		}	
		// Dupa ce stergem fisierele, mergem inapoi cu un director,
		// astfel child devine parent si parent "grandparent".
		child_directory = parent_directory;
		parent_directory = parent_directory->parent;
		
		// Stergem directorul copil din care am plecat
		if(child_directory->head_children_dirs != NULL) {
			free(child_directory->head_children_dirs->name);
			free(child_directory->head_children_dirs);
		}
		
		// Daca parintele depaseste directorul unde am executat functia
		// rmdir, atunci programul se opreste
		if (parent_directory == target) {
			free(parent_directory->name);
			free(parent_directory);
			return;
		}
		
		// Daca exista un alt director in directorul parinte,
		// reluam pasul 1 pentru acel director.
		// Daca nu exista, atunci verificam existenta fisierelor in
		// directorul parinte, asadar reluam pasul 2.
		if (child_directory->next != NULL) {
			parent_directory = child_directory->next;
			goto STEP1;
		} else {
			goto STEP2;
		}
	}
}

void rmdir(Dir *parent, char *name) {
	// Cazul 1 - directorul cautat este primul din directorul parinte
	if (parent->head_children_dirs != NULL &&
		!strcmp(parent->head_children_dirs->name, name)) {
		Dir *new_head;
		// new_head este setat pe al doilea director din lista, fie el si null
		new_head = parent->head_children_dirs->next;
		__rmdir(parent->head_children_dirs);
		parent->head_children_dirs = new_head;
		return;
	}

	Dir* current = parent->head_children_dirs;

	// Cazul 2 - directorul este diferit de primul din directorul parinte
	while (current != NULL) {
		Dir* prev = current;
		current = current->next;
		
		// Daca current a devenit null, deja am verificat daca
		// ultimul element din lista are numele cautat
		if (current == NULL)
			break;

		if (!strcmp(current->name, name)) {
			prev->next = current->next;
			if(current->next != NULL)
				current->next->parent = prev;
			__rmdir(current);
			return;
		}
	}

	if(current == NULL)
		printf("Could not find the dir\n");
}

void cd(Dir **target, char *name) {

	// Pentru "..", mergem in sus pe ierarhie
	if (!strcmp(name, "..")) {
		if((*target)->parent != NULL)
			(*target) = (*target)->parent;
		return;
	}

	// Daca avem un nume de director, iteram prin lista
	// si target devine directorul cautat
	Dir *current = (*target)->head_children_dirs;
	while (current != NULL) {
		if (!strcmp(current->name, name)) {
			(*target) = current;
			return;
		}
		current = current->next;
	}

	if (current == NULL)
		printf("No directories found!\n");
}

char *pwd(Dir *target) {
	Dir* current = target;
	char *to_concatenate = malloc(MAX_PATH_SIZE);
	char *to_print = malloc(MAX_PATH_SIZE);

	while(current != NULL) {
		strcpy(to_concatenate, "/");
		strcat(to_concatenate, current->name);
		strcat(to_concatenate, to_print);
		strcpy(to_print, to_concatenate);
		
		current = current->parent;
	}
	free(to_concatenate);
	return to_print;
}

void stop(Dir *target) {
	if (target->head_children_dirs != NULL) {
		Dir* current_directory = target->head_children_dirs;
		Dir* next_directory = current_directory->next;

		while (next_directory != NULL) {
			__rmdir(current_directory);
			current_directory = next_directory;
			next_directory = next_directory->next;
		}
		__rmdir(current_directory);
	}
	__rmdir(target);
}

void print_spaces(int level) {
	while(level > 0) {
		level --;
		printf("    ");
	}
}

void tree(Dir *target) {
	if(target->head_children_dirs == NULL) {
		return;
	}

	int level = 0;
	Dir* parent_directory = target->head_children_dirs;
	Dir* child_directory;
	File* nested_files;

	// Printam numele directorului parinte
STEP1:
	print_spaces(level);
	printf("%s\n", parent_directory->name);

	// Verfificam daca directorul curent (parinte) are un alt director
	// inauntrul sau. Daca are, printam directorul
	// si reluam pasul 1.
STEP2:
	child_directory = parent_directory->head_children_dirs;
	level ++;
	if (child_directory != NULL) {
		parent_directory = child_directory; 
		goto STEP1;
	} else {
STEP3:
		// Daca directorul curent (parinte) nu are alt director in el,
		// atunci printam fisierele
		nested_files = parent_directory->head_children_files;
		while (nested_files != NULL) {
			print_spaces(level);
			printf("%s\n", nested_files->name); 
			nested_files = nested_files->next;
		}
		// Dupa ce printam fisierele, mergem inapoi cu un director,
		// astfel child devine parent si parent "grandparent"
		child_directory = parent_directory;
		parent_directory = parent_directory->parent;
		level --;

		// Daca parintele depaseste directorul unde am executat functia
		// tree, atunci programul se opreste
		if (parent_directory == target->parent)
			return;
		
		// Daca exista un alt director in directorul parinte,
		// reluam pasul 1 pentru acel director.
		// Daca nu exista, atunci verificam existenta fisierelor in
		// directorul parinte, asadar reluam pasul 3.
		if (child_directory->next != NULL) {
			parent_directory = child_directory->next;
			goto STEP1;
		} else {
			goto STEP3;
		}
	}
}

void mv(Dir *parent, char *oldname, char *newname) {}

int main()
{
	// Declaram directorul home
	Dir *home = alloc_dir("home");

	// Alocam variabilele care citesc comenzile
	char *commands = malloc(MAX_INPUT_LINE_SIZE);
	DIE(commands == NULL, "Eroare alocare input comenzi!\n");
	char *function = malloc(MAX_INPUT_LINE_SIZE);
	DIE(function == NULL, "Eroare alocare input functii!\n");
	char *argument = malloc(MAX_INPUT_LINE_SIZE);
	DIE(argument == NULL, "Eroare alocare input argumente!\n");

	// Citim comenzile
	while (fgets(commands, MAX_INPUT_LINE_SIZE, stdin)) {

		sscanf(commands, "%s%s", function, argument);

		if (!strcmp(function, "touch")) {
			touch(home, argument);
		} else if (!strcmp(function, "mkdir")) {
			mkdir(home, argument);
		} else if (!strcmp(function, "ls")) {
			ls(home);
		} else if (!strcmp(function, "rm")) {
			rm(home, argument);
		} else if (!strcmp(function, "rmdir")) {
			rmdir(home, argument);
		} else if (!strcmp(function, "cd")) {
			cd(&home, argument);
		} else if (!strcmp(function, "tree")) {
			tree(home);
		} else if (!strcmp(function, "pwd")) {
			char *to_print = pwd(home);
			printf("%s\n", to_print);
			free(to_print);
		} else if (!strcmp(function, "stop")) {
			while(home->parent != NULL) {
				home = home->parent;
			}
			stop(home);
			// Dealocam main-ul
			free(commands);
			free(function);
			free(argument);
			break;
		}
	} 

	return 0;
}
