#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NAME_LEN 100
#define MAX_ISBN_LEN 20
#define MAX_DATE_LEN 11 // DD.MM.YYYY + null terminator
#define MAX_LINE_LEN 256

// Structure definitions
typedef struct Book {
    int bookId;
    char bookName[MAX_NAME_LEN];
    char ISBN[MAX_ISBN_LEN];
    struct BookExample *head; 
    struct Book *next;
} Book;

typedef struct BookExample {
    int exampleId;
    int status; // 0: On Shelf, 1: Borrowed
    struct BookExample *next;
} BookExample;

typedef struct Author {
    int authorId;
    char authorName[MAX_NAME_LEN];
    struct Author *next;
} Author;

typedef struct BookAuthor {
    int bookId;
    int authorId;
    struct BookAuthor *next;
} BookAuthor;

typedef struct Student {
    int studentId;
    char studentName[MAX_NAME_LEN];
    int penaltyDays;
    struct Student *next;
} Student;

typedef struct BookLoan {
    int loanId;
    int bookId;
    int exampleId;
    int studentId;
    char loanDate[MAX_DATE_LEN];
    char returnDate[MAX_DATE_LEN]; // Expected return date
    int returned; // 0: Not returned, 1: Returned
    struct BookLoan *next;
} BookLoan;

// Function prototypes 
void printMenu();
void loadBooks(Book **bookHead);
void saveBooks(Book *bookHead);
void addBook(Book **bookHead);
void deleteBook(Book **bookHead, BookLoan *loanHead);
void updateBook(Book *bookHead);
void printBooks(Book *bookHead);
Book *findBookById(Book *bookHead, int bookId);
Book *findBookByISBN(Book *bookHead, const char *ISBN);
Book *findBookByName(Book *bookHead, const char *bookName);
void createBookExamples(Book *bookHead);
void printBookExamples(Book *bookHead);
void printBookExamplesByBookName(Book *bookHead);
void updateBookExampleStatus(Book *bookHead, int bookId, int exampleId, int status);

void loadAuthors(Author **authorHead);
void saveAuthors(Author *authorHead);
void addAuthor(Author **authorHead);
void deleteAuthor(Author **authorHead, BookAuthor **bookAuthorArray, int *bookAuthorCount, int deletedAuthorId);
void updateAuthor(Author *authorHead);
void printAuthors(Author *authorHead);
Author *findAuthorById(Author *authorHead, int authorId);
Author *findAuthorByName(Author *authorHead, const char *authorName);

void loadBookAuthors(BookAuthor **bookAuthorArray, int *count);
void saveBookAuthors(BookAuthor *bookAuthorArray, int count);
void addBookAuthor(BookAuthor **bookAuthorArray, int *count, int bookId, int authorId);
void updateBookAuthor(BookAuthor *bookAuthorArray, int count);
void printBookAuthors(BookAuthor *bookAuthorArray, int count);
void updateBookAuthorAfterAuthorDeletion(BookAuthor **bookAuthorArray, int bookAuthorCount, int authorId);
void updateBookAuthorAfterBookDeletion(BookAuthor **bookAuthorArray, int bookAuthorCount, int bookId);


void loadStudents(Student **studentHead);
void saveStudents(Student *studentHead);
void addStudent(Student **studentHead);
void deleteStudentById(Student **studentHead, BookLoan *loanHead);
void deleteStudentByName(Student **studentHead, BookLoan *loanHead);
void updateStudent(Student *studentHead);
void printStudents(Student *studentHead);
Student *findStudentById(Student *studentHead, int studentId);
Student *findStudentByName(Student *studentHead, const char *studentName);
void printStudentInfo(Student *studentHead, BookLoan *loanHead);
void printStudentBookLoans(BookLoan *loanHead, int studentId);
void printStudentsWithPenalty(Student *studentHead);


void loadBookLoans(BookLoan **loanHead);
void saveBookLoans(BookLoan *loanHead);
void addBookLoan(BookLoan **loanHead, Book *bookHead);
void returnBook(BookLoan **loanHead, Book *bookHead);
void printBookLoans(BookLoan *loanHead);
void printOverdueLoans(BookLoan *loanHead);
int isBookReturned(BookLoan *loanHead, int bookId, int exampleId);
int getLoanDuration(BookLoan *loanHead, int loanId);
int getLoanCountForStudent(BookLoan *loanHead, int studentId);


void getCurrentDate(char *dateStr) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(dateStr, "%02d.%02d.%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}


int calculateDays(const char *date1Str, const char *date2Str) {
    struct tm tm1 = {0}, tm2 = {0};
    sscanf(date1Str, "%d.%d.%d", &tm1.tm_mday, &tm1.tm_mon, &tm1.tm_year);
    sscanf(date2Str, "%d.%d.%d", &tm2.tm_mday, &tm2.tm_mon, &tm2.tm_year);
    tm1.tm_mon -= 1; // struct tm months are 0-indexed
    tm2.tm_mon -= 1;
    tm1.tm_year -= 1900; // struct tm years are since 1900
    tm2.tm_year -= 1900;

    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    if (t1 == (time_t)-1 || t2 == (time_t)-1) {
        return -1; // Error in date conversion
    }

    double seconds = difftime(t2, t1);
    return (int)(seconds / (60 * 60 * 24));
}


// Free allocated memory
void freeBookLoans(BookLoan *head) {
    BookLoan *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void freeStudents(Student *head) {
    Student *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void freeAuthors(Author *head) {
    Author *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void freeBookExamples(BookExample *head) {
    BookExample *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void freeBooks(Book *head) {
    Book *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        freeBookExamples(temp->head); // Free book examples for this book
        free(temp);
    }
}


// --- Book Loan Functions ---

// Load book loans from CSV
void loadBookLoans(BookLoan **loanHead) {
    FILE *file = fopen("kitap_odunc.csv", "r");
    if (!file) {
        *loanHead = NULL;
        return;
    }

    char line[MAX_LINE_LEN];
    BookLoan *last = NULL;

    // Skip header row
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        BookLoan *newLoan = (BookLoan *)malloc(sizeof(BookLoan));
        if (!newLoan) {
            perror("Memory allocation failed");
            break; // Exit loop on allocation failure
        }
        newLoan->next = NULL;

        // Parse CSV line: loanId,bookId,exampleId,studentId,loanDate,returnDate,returned
        sscanf(line, "%d,%d,%d,%d,%[^,],%[^,],%d",
               &newLoan->loanId, &newLoan->bookId, &newLoan->exampleId, &newLoan->studentId,
               newLoan->loanDate, newLoan->returnDate, &newLoan->returned);

        if (*loanHead == NULL) {
            *loanHead = newLoan;
            last = newLoan;
        } else {
            last->next = newLoan;
            last = newLoan;
        }
    }
    fclose(file);
}

// Save book loans to CSV
void saveBookLoans(BookLoan *loanHead) {
    FILE *file = fopen("kitap_odunc.csv", "w");
    if (!file) {
        perror("Error opening kitap_odunc.csv for writing");
        return;
    }

    // Write header
    fprintf(file, "loanId,bookId,exampleId,studentId,loanDate,returnDate,returned\n");

    BookLoan *current = loanHead;
    while (current != NULL) {
        fprintf(file, "%d,%d,%d,%d,%s,%s,%d\n",
                current->loanId, current->bookId, current->exampleId, current->studentId,
                current->loanDate, current->returnDate, current->returned);
        current = current->next;
    }
    fclose(file);
}

// Add a new book loan
void addBookLoan(BookLoan **loanHead, Book *bookHead) {
    int studentId, bookId, exampleId;
    char loanDate[MAX_DATE_LEN];
    char returnDate[MAX_DATE_LEN];

    printf("Enter Student ID: ");
    scanf("%d", &studentId);
    getchar(); 

    printf("Enter Book ID: ");
    scanf("%d", &bookId);
    getchar(); 

    printf("Enter Example ID: ");
    scanf("%d", &exampleId);
    getchar(); 

    // Check if the book example is available
    Book *book = findBookById(bookHead, bookId);
    if (!book) {
        printf("Book not found.\n");
        return;
    }
    BookExample *example = book->head;
    while (example != NULL && example->exampleId != exampleId) {
        example = example->next;
    }
    if (!example || example->status == 1) {
        printf("Book example not available for loan.\n");
        return;
    }

    getCurrentDate(loanDate);
    // Calculate return date 
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    tm.tm_mday += 14; // Add 14 days
    mktime(&tm); // Normalize the date
    sprintf(returnDate, "%02d.%02d.%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);


    BookLoan *newLoan = (BookLoan *)malloc(sizeof(BookLoan));
    if (!newLoan) {
        perror("Memory allocation failed");
        return;
    }
    newLoan->next = NULL;

    // Find the next available loan ID
    int maxLoanId = 0;
    BookLoan *current = *loanHead;
    while (current != NULL) {
        if (current->loanId > maxLoanId) {
            maxLoanId = current->loanId;
        }
        current = current->next;
    }
    newLoan->loanId = maxLoanId + 1;

    newLoan->bookId = bookId;
    newLoan->exampleId = exampleId;
    newLoan->studentId = studentId;
    strcpy(newLoan->loanDate, loanDate);
    strcpy(newLoan->returnDate, returnDate);
    newLoan->returned = 0; // Not returned yet

    // Add to the end of the list
    if (*loanHead == NULL) {
        *loanHead = newLoan;
    } else {
        current = *loanHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newLoan;
    }

    // Update book example status
    updateBookExampleStatus(bookHead, bookId, exampleId, 1); // Set status to borrowed

    printf("Book loaned successfully.\n");
}

// Return a book
void returnBook(BookLoan **loanHead, Book *bookHead) {
    int loanId;
    printf("Enter Loan ID to return: ");
    scanf("%d", &loanId);
    getchar(); 

    BookLoan *current = *loanHead;
    BookLoan *prev = NULL;
    while (current != NULL && current->loanId != loanId) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        printf("Loan with ID %d not found.\n", loanId);
        return;
    }

    if (current->returned == 1) {
        printf("Book for Loan ID %d has already been returned.\n", loanId);
        return;
    }

    // Update loan status
    current->returned = 1;

    // Update book example status
    updateBookExampleStatus(bookHead, current->bookId, current->exampleId, 0); // Set status to on Shelf

    printf("Book returned successfully.\n");
}


// Print all book loans
void printBookLoans(BookLoan *loanHead) {
    if (!loanHead) {
        printf("No book loans recorded.\n");
        return;
    }
    printf("\n--- Book Loans ---\n");
    printf("ID | Book ID | Example ID | Student ID | Loan Date | Return Date | Returned\n");
    printf("---|---------|------------|------------|-----------|-------------|---------\n");
    BookLoan *tmp = loanHead;
    while (tmp != NULL) {
        printf("%-2d | %-7d | %-10d | %-10d | %-9s | %-11s | %d\n",
               tmp->loanId, tmp->bookId, tmp->exampleId, tmp->studentId,
               tmp->loanDate, tmp->returnDate, tmp->returned);
        tmp = tmp->next;
    }
    printf("-------------------\n");
}

// Print overdue book loans
void printOverdueLoans(BookLoan *loanHead) {
    char currentDate[MAX_DATE_LEN];
    getCurrentDate(currentDate);

    printf("\n--- Overdue Book Loans ---\n");
    printf("ID | Book ID | Example ID | Student ID | Loan Date | Return Date\n");
    printf("---|---------|------------|------------|-----------|-------------\n");

    BookLoan *tmp = loanHead;
    int foundOverdue = 0;
    while (tmp != NULL) {
        if (tmp->returned == 0) {
            int days = calculateDays(tmp->returnDate, currentDate);
            if (days > 0) {
                printf("%-2d | %-7d | %-10d | %-10d | %-9s | %-11s\n",
                       tmp->loanId, tmp->bookId, tmp->exampleId, tmp->studentId,
                       tmp->loanDate, tmp->returnDate);
                foundOverdue = 1;
            }
        }
        tmp = tmp->next;
    }

    if (!foundOverdue) {
        printf("No overdue book loans.\n");
    }
    printf("--------------------------\n");
}


// Check if a specific book example is returned
int isBookReturned(BookLoan *loanHead, int bookId, int exampleId) {
    BookLoan *temp = loanHead;
    while (temp != NULL) {
        if (temp->bookId == bookId && temp->exampleId == exampleId && temp->returned == 0) {
            return 0; // Found an active loan for this example
        }
        temp = temp->next;
    }
    return 1; // No active loan found, assumed returned or never borrowed
}

// Get the duration of a loan in days
int getLoanDuration(BookLoan *loanHead, int loanId) {
    BookLoan *temp = loanHead;
    while (temp != NULL) {
        if (temp->loanId == loanId) {
            char returnDate[MAX_DATE_LEN];
            getCurrentDate(returnDate); // Use current date for calculation
            return calculateDays(temp->loanDate, returnDate);
        }
        temp = temp->next;
    }
    return -1; // Loan not found
}

// Get the number of active loans for a student
int getLoanCountForStudent(BookLoan *loanHead, int studentId) {
    int count = 0;
    BookLoan *temp = loanHead;
    while (temp != NULL) {
        if (temp->studentId == studentId && temp->returned == 0) {
            count++;
        }
        temp = temp->next;
    }
    return count;
}


// --- Book Functions ---

// Load books from CSV
void loadBooks(Book **bookHead) {
    FILE *file = fopen("kitaplar.csv", "r");
    if (!file) {
        *bookHead = NULL;
        return;
    }

    char line[MAX_LINE_LEN];
    Book *last = NULL;

    // Skip header row
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        Book *newBook = (Book *)malloc(sizeof(Book));
        if (!newBook) {
            perror("Memory allocation failed");
            break; 
        }
        newBook->next = NULL;
        newBook->head = NULL; // Initialize book examples head

        // Parse CSV line: bookId,bookName,ISBN,exampleCount
        int exampleCount;
        sscanf(line, "%d,%[^,],%[^,],%d",
               &newBook->bookId, newBook->bookName, newBook->ISBN, &exampleCount);

        // Create book examples
        BookExample *lastExample = NULL;
        for (int i = 0; i < exampleCount; i++) {
            BookExample *newExample = (BookExample *)malloc(sizeof(BookExample));
            if (!newExample) {
                perror("Memory allocation failed");
                break; 
            }
            newExample->exampleId = i + 1;
            newExample->status = 0; // Default status: On Shelf
            newExample->next = NULL;

            if (newBook->head == NULL) {
                newBook->head = newExample;
                lastExample = newExample;
            } else {
                lastExample->next = newExample;
                lastExample = newExample;
            }
        }

        if (*bookHead == NULL) {
            *bookHead = newBook;
            last = newBook;
        } else {
            last->next = newBook;
            last = newBook;
        }
    }
    fclose(file);
}

// Save books to CSV
void saveBooks(Book *bookHead) {
    FILE *file = fopen("kitaplar.csv", "w");
    if (!file) {
        perror("Error opening kitaplar.csv for writing");
        return;
    }

    // Write header
    fprintf(file, "bookId,bookName,ISBN,exampleCount\n");

    Book *currentBook = bookHead;
    while (currentBook != NULL) {
        // Count book examples
        int exampleCount = 0;
        BookExample *currentExample = currentBook->head;
        while (currentExample != NULL) {
            exampleCount++;
            currentExample = currentExample->next;
        }
        fprintf(file, "%d,%s,%s,%d\n",
                currentBook->bookId, currentBook->bookName, currentBook->ISBN, exampleCount);
        currentBook = currentBook->next;
    }
    fclose(file);
}


// Add a new book
void addBook(Book **bookHead) {
    Book *newBook = (Book *)malloc(sizeof(Book));
    if (!newBook) {
        perror("Memory allocation failed");
        return;
    }
    newBook->next = NULL;
    newBook->head = NULL; // Initialize book examples head

    // Find the next available book ID
    int maxBookId = 0;
    Book *current = *bookHead;
    while (current != NULL) {
        if (current->bookId > maxBookId) {
            maxBookId = current->bookId;
        }
        current = current->next;
    }
    newBook->bookId = maxBookId + 1;

    printf("Enter Book Name: ");
    fgets(newBook->bookName, sizeof(newBook->bookName), stdin);
    newBook->bookName[strcspn(newBook->bookName, "\n")] = 0; 

    printf("Enter ISBN: ");
    fgets(newBook->ISBN, sizeof(newBook->ISBN), stdin);
    newBook->ISBN[strcspn(newBook->ISBN, "\n")] = 0; 

    int exampleCount;
    printf("Enter Number of Examples: ");
    scanf("%d", &exampleCount);
    getchar(); 

    // Create book examples
    BookExample *lastExample = NULL;
    for (int i = 0; i < exampleCount; i++) {
        BookExample *newExample = (BookExample *)malloc(sizeof(BookExample));
        if (!newExample) {
            perror("Memory allocation failed");
            break; 
        }
        newExample->exampleId = i + 1;
        newExample->status = 0; // Default status on shelf
        newExample->next = NULL;

        if (newBook->head == NULL) {
            newBook->head = newExample;
            lastExample = newExample;
        }
        else {
            lastExample->next = newExample;
            lastExample = newExample;
        }
    }


    // Add to the end of the list
    if (*bookHead == NULL) {
        *bookHead = newBook;
    } else {
        current = *bookHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newBook;
    }

    printf("Book added successfully with ID %d.\n", newBook->bookId);
}

// Delete a book
void deleteBook(Book **bookHead, BookLoan *loanHead) {
    int bookId;
    printf("Enter Book ID to delete: ");
    scanf("%d", &bookId);
    getchar(); 

    Book *current = *bookHead;
    Book *prev = NULL;
    while (current != NULL && current->bookId != bookId) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        printf("Book with ID %d not found.\n", bookId);
        return;
    }

    // Check if any examples of this book are currently borrowed
    BookExample *example = current->head;
    while (example != NULL) {
        if (example->status == 1) {
            printf("Cannot delete book. Some examples are currently borrowed.\n");
            return;
        }
        example = example->next;
    }


    if (prev == NULL) {
        *bookHead = current->next; // Deleting the head
    } else {
        prev->next = current->next;
    }

    freeBookExamples(current->head); // Free book examples
    free(current); // Free the book node


    printf("Book with ID %d deleted successfully.\n", bookId);
}

// Update book information
void updateBook(Book *bookHead) {
    int bookId;
    printf("Enter Book ID to update: ");
    scanf("%d", &bookId);
    getchar(); 

    Book *book = findBookById(bookHead, bookId);
    if (!book) {
        printf("Book with ID %d not found.\n", bookId);
        return;
    }

    printf("Enter new Book Name (leave blank to keep current '%s'): ", book->bookName);
    char newBookName[MAX_NAME_LEN];
    fgets(newBookName, sizeof(newBookName), stdin);
    newBookName[strcspn(newBookName, "\n")] = 0; 
    if (strlen(newBookName) > 0) {
        strcpy(book->bookName, newBookName);
    }

    printf("Enter new ISBN (leave blank to keep current '%s'): ", book->ISBN);
    char newISBN[MAX_ISBN_LEN];
    fgets(newISBN, sizeof(newISBN), stdin);
    newISBN[strcspn(newISBN, "\n")] = 0; 
    if (strlen(newISBN) > 0) {
        strcpy(book->ISBN, newISBN);
    }

    printf("Book with ID %d updated successfully.\n", bookId);
}

// Print all books
void printBooks(Book *bookHead) {
    if (!bookHead) {
        printf("No books in the library.\n");
        return;
    }
    printf("\n--- Books ---\n");
    printf("ID | Book Name%*s | ISBN%*s | Example Count\n", MAX_NAME_LEN - 10, "", MAX_ISBN_LEN - 4, "");
    printf("---|-----------%*s|------%*s|---------------\n", MAX_NAME_LEN - 10, "", MAX_ISBN_LEN - 4, "");
    Book *temp = bookHead;
    while (temp != NULL) {
        int exampleCount = 0;
        BookExample *example = temp->head;
        while(example != NULL) {
            exampleCount++;
            example = example->next;
        }
        printf("%-2d | %-*s | %-*s | %d\n",
               temp->bookId, MAX_NAME_LEN - 1, temp->bookName, MAX_ISBN_LEN - 1, temp->ISBN, exampleCount);
        temp = temp->next;
    }
    printf("-------------\n");
}

// Find a book by ID
Book *findBookById(Book *bookHead, int bookId) {
    Book *temp = bookHead;
    while (temp != NULL) {
        if (temp->bookId == bookId) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}

// Find a book by ISBN
Book *findBookByISBN(Book *bookHead, const char *ISBN) {
    Book *temp = bookHead;
    while (temp != NULL) {
        if (strcmp(temp->ISBN, ISBN) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}

// Find a book by name
Book *findBookByName(Book *bookHead, const char *bookName) {
    Book *temp = bookHead;
    while (temp != NULL) {
        if (strcmp(temp->bookName, bookName) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}


// // Create book examples (called during book loading/adding)
// void createBookExamples(Book *bookHead) {
// }

// Print all book examples for all books
void printBookExamples(Book *bookHead) {
    if (!bookHead) {
        printf("No books in the library to show examples.\n");
        return;
    }
    printf("\n--- Book Examples ---\n");
    Book *tmp = bookHead;
    while (tmp != NULL) {
        printf("Book: %s (ID: %d)\n", tmp->bookName, tmp->bookId);
        printf("  Example ID | Status (0: Shelf, 1: Borrowed)\n");
        printf("  -----------|-------------------------------\n");
        BookExample *tmpExample = tmp->head;
        while (tmpExample != NULL) {
            printf("  %-10d | %d\n", tmpExample->exampleId, tmpExample->status);
            tmpExample = tmpExample->next;
        }
        printf("----------------------------------------\n");
        tmp = tmp->next;
    }
}

// Print book examples for a specific book by name
void printBookExamplesByBookName(Book *bookHead) {
    char bookName[MAX_NAME_LEN];
    printf("Enter Book Name to show examples: ");
    fgets(bookName, sizeof(bookName), stdin);
    bookName[strcspn(bookName, "\n")] = 0; 

    Book *book = findBookByName(bookHead, bookName);
    if (!book) {
        printf("Book '%s' not found.\n", bookName);
        return;
    }

    printf("\n--- Book Examples for '%s' ---\n", book->bookName);
    printf("  Example ID | Status (0: Shelf, 1: Borrowed)\n");
    printf("  -----------|-------------------------------\n");
    BookExample *tmpExample = book->head;
    if (!tmpExample) {
        printf("  No examples available for this book.\n");
    } else {
        while (tmpExample != NULL) {
            printf("  %-10d | %d\n", tmpExample->exampleId, tmpExample->status);
            tmpExample = tmpExample->next;
        }
    }
    printf("----------------------------------------\n");
}


// Update the status of a specific book example
void updateBookExampleStatus(Book *bookHead, int bookId, int exampleId, int status) {
    Book *book = findBookById(bookHead, bookId);
    if (!book) {
        printf("Error: Book not found for example status update.\n");
        return;
    }

    BookExample *example = book->head;
    while (example != NULL && example->exampleId != exampleId) {
        example = example->next;
    }

    if (!example) {
        printf("Error: Book example not found for status update.\n");
        return;
    }

    example->status = status;
}


// --- Author Functions ---

// Load authors from CSV
void loadAuthors(Author **authorHead) {
    FILE *file = fopen("yazarlar.csv", "r");
    if (!file) {
        *authorHead = NULL;
        return;
    }

    char line[MAX_LINE_LEN];
    Author *last = NULL;

    
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        Author *newAuthor = (Author *)malloc(sizeof(Author));
        if (!newAuthor) {
            perror("Memory allocation failed");
            break; 
        }
        newAuthor->next = NULL;

        // Parse CSV line: authorId,authorName
        sscanf(line, "%d,%[^\n]", &newAuthor->authorId, newAuthor->authorName);

        if (*authorHead == NULL) {
            *authorHead = newAuthor;
            last = newAuthor;
        } else {
            last->next = newAuthor;
            last = newAuthor;
        }
    }
    fclose(file);
}

// Save authors to CSV
void saveAuthors(Author *authorHead) {
    FILE *file = fopen("yazarlar.csv", "w");
    if (!file) {
        perror("Error opening yazarlar.csv for writing");
        return;
    }

    // Write header
    fprintf(file, "authorId,authorName\n");

    Author *current = authorHead;
    while (current != NULL) {
        fprintf(file, "%d,%s\n", current->authorId, current->authorName);
        current = current->next;
    }
    fclose(file);
}

// Add a new author
void addAuthor(Author **authorHead) {
    Author *newAuthor = (Author *)malloc(sizeof(Author));
    if (!newAuthor) {
        perror("Memory allocation failed");
        return;
    }
    newAuthor->next = NULL;

    // Find the next available author ID
    int maxAuthorId = 0;
    Author *current = *authorHead;
    while (current != NULL) {
        if (current->authorId > maxAuthorId) {
            maxAuthorId = current->authorId;
        }
        current = current->next;
    }
    newAuthor->authorId = maxAuthorId + 1;

    printf("Enter Author Name: ");
    fgets(newAuthor->authorName, sizeof(newAuthor->authorName), stdin);
    newAuthor->authorName[strcspn(newAuthor->authorName, "\n")] = 0; 

    // Add to the end of the list
    if (*authorHead == NULL) {
        *authorHead = newAuthor;
    } else {
        current = *authorHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newAuthor;
    }

    printf("Author added successfully with ID %d.\n", newAuthor->authorId);
}

// Delete an author
void deleteAuthor(Author **authorHead, BookAuthor **bookAuthorArray, int *bookAuthorCount, int deletedAuthorId) {
    Author *current = *authorHead;
    Author *prev = NULL;
    while (current != NULL && current->authorId != deletedAuthorId) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        printf("Author with ID %d not found.\n", deletedAuthorId);
        return;
    }

    if (prev == NULL) {
        *authorHead = current->next; 
    } else {
        prev->next = current->next;
    }

    free(current); 

   
    updateBookAuthorAfterAuthorDeletion(bookAuthorArray, *bookAuthorCount, deletedAuthorId);


    printf("Author with ID %d deleted successfully.\n", deletedAuthorId);
}


// Update author information
void updateAuthor(Author *authorHead) {
    int authorId;
    printf("Enter Author ID to update: ");
    scanf("%d", &authorId);
    getchar(); 

    Author *author = findAuthorById(authorHead, authorId);
    if (!author) {
        printf("Author with ID %d not found.\n", authorId);
        return;
    }

    printf("Enter new Author Name (leave blank to keep current '%s'): ", author->authorName);
    char newAuthorName[MAX_NAME_LEN];
    fgets(newAuthorName, sizeof(newAuthorName), stdin);
    newAuthorName[strcspn(newAuthorName, "\n")] = 0; 
    if (strlen(newAuthorName) > 0) {
        strcpy(author->authorName, newAuthorName);
    }

    printf("Author with ID %d updated successfully.\n", authorId);
}

// Print all authors
void printAuthors(Author *authorHead) {
    if (!authorHead) {
        printf("No authors in the system.\n");
        return;
    }
    printf("\n--- Authors ---\n");
    printf("ID | Author Name\n");
    printf("---|------------\n");
    Author *temp = authorHead;
    while (temp != NULL) {
        printf("%-2d | %s\n", temp->authorId, temp->authorName);
        temp = temp->next;
    }
    printf("---------------\n");
}

// Find an author by ID
Author *findAuthorById(Author *authorHead, int authorId) {
    Author *temp = authorHead;
    while (temp != NULL) {
        if (temp->authorId == authorId) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}

// Find an author by name
Author *findAuthorByName(Author *authorHead, const char *authorName) {
    Author *temp = authorHead;
    while (temp != NULL) {
        if (strcmp(temp->authorName, authorName) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}

// --- Book-Author Link Functions ---

// Load book-author links from CSV
void loadBookAuthors(BookAuthor **bookAuthorArray, int *count) {
    FILE *file = fopen("kitap_yazar.csv", "r");
    if (!file) {
        *bookAuthorArray = NULL;
        *count = 0;
        return;
    }

    char line[MAX_LINE_LEN];
    *count = 0;

    // Count lines to determine array size (excluding header)
    while (fgets(line, sizeof(line), file)) {
        (*count)++;
    }
    fseek(file, 0, SEEK_SET); // Reset file pointer

    if (*count > 1) { // If there are entries besides the header
        *bookAuthorArray = (BookAuthor *)malloc(sizeof(BookAuthor) * (*count - 1));
        if (!*bookAuthorArray) {
            perror("Memory allocation failed");
            *count = 0;
            fclose(file);
            return;
        }

        
        fgets(line, sizeof(line), file);

        int i = 0;
        while (fgets(line, sizeof(line), file)) {
            sscanf(line, "%d,%d", &(*bookAuthorArray)[i].bookId, &(*bookAuthorArray)[i].authorId);
            i++;
        }
        *count = i; // Update count to actual number of entries
    } else {
        *bookAuthorArray = NULL;
        *count = 0;
    }

    fclose(file);
}

// Save book-author links to CSV
void saveBookAuthors(BookAuthor *bookAuthorArray, int count) {
    FILE *file = fopen("kitap_yazar.csv", "w");
    if (!file) {
        perror("Error opening kitap_yazar.csv for writing");
        return;
    }

    // Write header
    fprintf(file, "bookId,authorId\n");

    for (int i = 0; i < count; i++) {
        fprintf(file, "%d,%d\n", bookAuthorArray[i].bookId, bookAuthorArray[i].authorId);
    }
    fclose(file);
}


// Add a new book-author link
void addBookAuthor(BookAuthor **bookAuthorArray, int *count, int bookId, int authorId) {
    // Check if the link already exists
    for (int i = 0; i < *count; i++) {
        if ((*bookAuthorArray)[i].bookId == bookId && (*bookAuthorArray)[i].authorId == authorId) {
            printf("This book-author link already exists.\n");
            return;
        }
    }

    // Resize the array
    *bookAuthorArray = (BookAuthor *)realloc(*bookAuthorArray, sizeof(BookAuthor) * (*count + 1));
    if (!*bookAuthorArray) {
        perror("Memory re-allocation failed");
        return;
    }

    // Add the new link
    (*bookAuthorArray)[*count].bookId = bookId;
    (*bookAuthorArray)[*count].authorId = authorId;
    (*count)++;

    printf("Book-author link added successfully.\n");
}

// Update a book-author link (Less common, usually delete and re-add)
void updateBookAuthor(BookAuthor *bookAuthorArray, int count) {
    printf("Updating book-author links is typically done by deleting and re-adding.\n");
}

// Print all book-author links
void printBookAuthors(BookAuthor *bookAuthorArray, int count) {
    if (count == 0) {
        printf("No book-author links recorded.\n");
        return;
    }
    printf("\n--- Book-Author Links ---\n");
    printf("Book ID | Author ID\n");
    printf("--------|----------\n");
    for (int i = 0; i < count; i++) {
        printf("%-7d | %d\n", bookAuthorArray[i].bookId, bookAuthorArray[i].authorId);
    }
    printf("-------------------------\n");
}

// Update book-author array after an author is deleted
void updateBookAuthorAfterAuthorDeletion(BookAuthor **bookAuthorArray, int bookAuthorCount, int authorId) {
    int newCount = 0;
    for (int i = 0; i < bookAuthorCount; i++) {
        if ((*bookAuthorArray)[i].authorId != authorId) {
            (*bookAuthorArray)[newCount] = (*bookAuthorArray)[i];
            newCount++;
        }
    }
    // Resize the array if necessary (optional, but good practice)
    if (newCount < bookAuthorCount) {
        *bookAuthorArray = (BookAuthor *)realloc(*bookAuthorArray, sizeof(BookAuthor) * newCount);
        if (!*bookAuthorArray && newCount > 0) {
             perror("Memory re-allocation failed after author deletion");
        }
    }
}

// Update book-author array after a book is deleted
void updateBookAuthorAfterBookDeletion(BookAuthor **bookAuthorArray, int bookAuthorCount, int bookId) {
    int newCount = 0;
    for (int i = 0; i < bookAuthorCount; i++) {
        if ((*bookAuthorArray)[i].bookId != bookId) {
            (*bookAuthorArray)[newCount] = (*bookAuthorArray)[i];
            newCount++;
        }
    }
    // Resize the array if necessary (optional, but good practice)
    if (newCount < bookAuthorCount) {
         *bookAuthorArray = (BookAuthor *)realloc(*bookAuthorArray, sizeof(BookAuthor) * newCount);
         if (!*bookAuthorArray && newCount > 0) {
             perror("Memory re-allocation failed after book deletion");
             // Handle error
         }
    }
}


// --- Student Functions ---

// Load students from CSV
void loadStudents(Student **studentHead) {
    FILE *file = fopen("ogrenciler.csv", "r");
    if (!file) {
        *studentHead = NULL;
        return;
    }

    char line[MAX_LINE_LEN];
    Student *last = NULL;

    
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        Student *newStudent = (Student *)malloc(sizeof(Student));
        if (!newStudent) {
            perror("Memory allocation failed");
            break; 
        }
        newStudent->next = NULL;

        // Parse CSV line: studentId,studentName,penaltyDays
        sscanf(line, "%d,%[^,],%d", &newStudent->studentId, newStudent->studentName, &newStudent->penaltyDays);

        if (*studentHead == NULL) {
            *studentHead = newStudent;
            last = newStudent;
        } else {
            last->next = newStudent;
            last = newStudent;
        }
    }
    fclose(file);
}

// Save students to CSV
void saveStudents(Student *studentHead) {
    FILE *file = fopen("ogrenciler.csv", "w");
    if (!file) {
        perror("Error opening ogrenciler.csv for writing");
        return;
    }

    // Write header
    fprintf(file, "studentId,studentName,penaltyDays\n");

    Student *current = studentHead;
    while (current != NULL) {
        fprintf(file, "%d,%s,%d\n", current->studentId, current->studentName, current->penaltyDays);
        current = current->next;
    }
    fclose(file);
}

// Add a new student
void addStudent(Student **studentHead) {
    Student *newStudent = (Student *)malloc(sizeof(Student));
    if (!newStudent) {
        perror("Memory allocation failed");
        return;
    }
    newStudent->next = NULL;

    // Find the next available student ID
    int maxStudentId = 0;
    Student *current = *studentHead;
    while (current != NULL) {
        if (current->studentId > maxStudentId) {
            maxStudentId = current->studentId;
        }
        current = current->next;
    }
    newStudent->studentId = maxStudentId + 1;

    printf("Enter Student Name: ");
    fgets(newStudent->studentName, sizeof(newStudent->studentName), stdin);
    newStudent->studentName[strcspn(newStudent->studentName, "\n")] = 0; 

    newStudent->penaltyDays = 0; // New student starts with 0 penalty days

    // Add to the end of the list
    if (*studentHead == NULL) {
        *studentHead = newStudent;
    } else {
        current = *studentHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newStudent;
    }

    printf("Student added successfully with ID %d.\n", newStudent->studentId);
}

// Delete a student by ID
void deleteStudentById(Student **studentHead, BookLoan *loanHead) {
    int studentId;
    printf("Enter Student ID to delete: ");
    scanf("%d", &studentId);
    getchar(); 

    // Check if the student has any active loans
    if (getLoanCountForStudent(loanHead, studentId) > 0) {
        printf("Cannot delete student with active book loans.\n");
        return;
    }

    Student *current = *studentHead;
    Student *prev = NULL;
    while (current != NULL && current->studentId != studentId) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        printf("Student with ID %d not found.\n", studentId);
        return;
    }

    if (prev == NULL) {
        *studentHead = current->next; // Deleting the head
    } else {
        prev->next = current->next;
    }

    free(current); 

    printf("Student with ID %d deleted successfully.\n", studentId);
}

// Delete a student by Name
void deleteStudentByName(Student **studentHead, BookLoan *loanHead) {
    char studentName[MAX_NAME_LEN];
    printf("Enter Student Name to delete: ");
    fgets(studentName, sizeof(studentName), stdin);
    studentName[strcspn(studentName, "\n")] = 0; 

    Student *current = *studentHead;
    Student *prev = NULL;
    while (current != NULL && strcmp(current->studentName, studentName) != 0) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        printf("Student with name '%s' not found.\n", studentName);
        return;
    }

    // Check if the student has any active loans
    if (getLoanCountForStudent(loanHead, current->studentId) > 0) {
        printf("Cannot delete student with active book loans.\n");
        return;
    }


    if (prev == NULL) {
        *studentHead = current->next; // Deleting the head
    } else {
        prev->next = current->next;
    }

    free(current); 

    printf("Student with name '%s' deleted successfully.\n", studentName);
}


// Update student information
void updateStudent(Student *studentHead) {
    int studentId;
    printf("Enter Student ID to update: ");
    scanf("%d", &studentId);
    getchar(); 

    Student *student = findStudentById(studentHead, studentId);
    if (!student) {
        printf("Student with ID %d not found.\n", studentId);
        return;
    }

    printf("Enter new Student Name (leave blank to keep current '%s'): ", student->studentName);
    char newStudentName[MAX_NAME_LEN];
    fgets(newStudentName, sizeof(newStudentName), stdin);
    newStudentName[strcspn(newStudentName, "\n")] = 0; 
    if (strlen(newStudentName) > 0) {
        strcpy(student->studentName, newStudentName);
    }

    printf("Student with ID %d updated successfully.\n", studentId);
}

// Print all students
void printStudents(Student *studentHead) {
    if (!studentHead) {
        printf("No students in the system.\n");
        return;
    }
    printf("\n--- Students ---\n");
    printf("ID | Student Name%*s | Penalty Days\n", MAX_NAME_LEN - 12, "");
    printf("---|-------------%*s|--------------\n", MAX_NAME_LEN - 12, "");
    Student *temp = studentHead;
    while (temp != NULL) {
        printf("%-2d | %-*s | %d\n", temp->studentId, MAX_NAME_LEN - 1, temp->studentName, temp->penaltyDays);
        temp = temp->next;
    }
    printf("----------------\n");
}

// Find a student by ID
Student *findStudentById(Student *studentHead, int studentId) {
    Student *temp = studentHead;
    while (temp != NULL) {
        if (temp->studentId == studentId) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}

// Find a student by name
Student *findStudentByName(Student *studentHead, const char *studentName) {
    Student *temp = studentHead;
    while (temp != NULL) {
        if (strcmp(temp->studentName, studentName) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL; // Not found
}

// Print information for a specific student
void printStudentInfo(Student *studentHead, BookLoan *loanHead) {
    int studentId;
    printf("Enter Student ID to view info: ");
    scanf("%d", &studentId);
    getchar(); 

    Student *student = findStudentById(studentHead, studentId);
    if (!student) {
        printf("Student with ID %d not found.\n", studentId);
        return;
    }

    printf("\n--- Student Information ---\n");
    printf("ID: %d\n", student->studentId);
    printf("Name: %s\n", student->studentName);
    printf("Penalty Days: %d\n", student->penaltyDays);

    printf("\nActive Loans:\n");
    printStudentBookLoans(loanHead, studentId);

    printf("---------------------------\n");
}

// Print book loans for a specific student
void printStudentBookLoans(BookLoan *loanHead, int studentId) {
    int foundLoans = 0;
    BookLoan *tmp = loanHead;
    while (tmp != NULL) {
        if (tmp->studentId == studentId && tmp->returned == 0) {
            if (!foundLoans) {
                printf("  Loan ID | Book ID | Example ID | Loan Date | Return Date\n");
                printf("  --------|---------|------------|-----------|-------------\n");
                foundLoans = 1;
            }
            printf("  %-7d | %-7d | %-10d | %-9s | %-11s\n",
                   tmp->loanId, tmp->bookId, tmp->exampleId, tmp->loanDate, tmp->returnDate);
        }
        tmp = tmp->next;
    }
    if (!foundLoans) {
        printf("  No active loans for this student.\n");
    }
}

// Print students with penalty days
void printStudentsWithPenalty(Student *studentHead) {
    printf("\n--- Students with Penalty ---\n");
    printf("ID | Student Name%*s | Penalty Days\n", MAX_NAME_LEN - 12, "");
    printf("---|-------------%*s|--------------\n", MAX_NAME_LEN - 12, "");

    Student *temp = studentHead;
    int foundPenalty = 0;
    while (temp != NULL) {
        if (temp->penaltyDays > 0) {
            printf("%-2d | %-*s | %d\n", temp->studentId, MAX_NAME_LEN - 1, temp->studentName, temp->penaltyDays);
            foundPenalty = 1;
        }
        temp = temp->next;
    }

    if (!foundPenalty) {
        printf("No students currently have penalty days.\n");
    }
    printf("-----------------------------\n");
}


// --- Main Function and Menu ---

int main() {
    Book *bookHead = NULL;
    Author *authorHead = NULL;
    Student *studentHead = NULL;
    BookLoan *loanHead = NULL;
    BookAuthor *bookAuthorArray = NULL;
    int bookAuthorCount = 0;

    // Load data from CSV files
    loadBooks(&bookHead);
    loadAuthors(&authorHead);
    loadStudents(&studentHead);
    loadBookLoans(&loanHead);
    loadBookAuthors(&bookAuthorArray, &bookAuthorCount);

    int choice;
    do {
        printMenu();
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); 

        switch (choice) {
            case 1: // Book Operations
                printf("\n--- Book Operations ---\n");
                printf("1. Add Book\n");
                printf("2. Delete Book\n");
                printf("3. Update Book\n");
                printf("4. List All Books\n");
                printf("5. List Book Examples (All Books)\n");
                printf("6. List Book Examples (By Book Name)\n");
                printf("7. Find Book by Name\n");
                printf("8. Find Book by ISBN\n");
                printf("9. Back to Main Menu\n");
                printf("Enter your choice: ");
                int bookChoice;
                scanf("%d", &bookChoice);
                getchar(); 

                switch (bookChoice) {
                    case 1: addBook(&bookHead); break;
                    case 2: deleteBook(&bookHead, loanHead); break;
                    case 3: updateBook(bookHead); break;
                    case 4: printBooks(bookHead); break;
                    case 5: printBookExamples(bookHead); break;
                    case 6: printBookExamplesByBookName(bookHead); break;
                    case 7: {
                        char bookName[MAX_NAME_LEN];
                        printf("Enter Book Name to find: ");
                        fgets(bookName, sizeof(bookName), stdin);
                        bookName[strcspn(bookName, "\n")] = 0;
                        Book *foundBook = findBookByName(bookHead, bookName);
                        if (foundBook) {
                            printf("Book Found: ID %d, Name: %s, ISBN: %s\n", foundBook->bookId, foundBook->bookName, foundBook->ISBN);
                        } else {
                            printf("Book '%s' not found.\n", bookName);
                        }
                        break;
                    }
                    case 8: {
                         char ISBN[MAX_ISBN_LEN];
                         printf("Enter ISBN to find: ");
                         fgets(ISBN, sizeof(ISBN), stdin);
                         ISBN[strcspn(ISBN, "\n")] = 0;
                         Book *foundBook = findBookByISBN(bookHead, ISBN);
                         if (foundBook) {
                             printf("Book Found: ID %d, Name: %s, ISBN: %s\n", foundBook->bookId, foundBook->bookName, foundBook->ISBN);
                         } else {
                             printf("Book with ISBN '%s' not found.\n", ISBN);
                         }
                         break;
                    }
                    case 9: break; 
                    default: printf("Invalid choice.\n");
                }
                break;

            case 2: // Author Operations
                 printf("\n--- Author Operations ---\n");
                 printf("1. Add Author\n");
                 printf("2. Delete Author\n");
                 printf("3. Update Author\n");
                 printf("4. List All Authors\n");
                 printf("5. Find Author by Name\n");
                 printf("6. Back to Main Menu\n");
                 printf("Enter your choice: ");
                 int authorChoice;
                 scanf("%d", &authorChoice);
                 getchar(); 

                 switch (authorChoice) {
                     case 1: addAuthor(&authorHead); break;
                     case 2: {
                         int authorIdToDelete;
                         printf("Enter Author ID to delete: ");
                         scanf("%d", &authorIdToDelete);
                         getchar(); 
                         deleteAuthor(&authorHead, &bookAuthorArray, &bookAuthorCount, authorIdToDelete);
                         break;
                     }
                     case 3: updateAuthor(authorHead); break;
                     case 4: printAuthors(authorHead); break;
                     case 5: {
                         char authorName[MAX_NAME_LEN];
                         printf("Enter Author Name to find: ");
                         fgets(authorName, sizeof(authorName), stdin);
                         authorName[strcspn(authorName, "\n")] = 0;
                         Author *foundAuthor = findAuthorByName(authorHead, authorName);
                         if (foundAuthor) {
                             printf("Author Found: ID %d, Name: %s\n", foundAuthor->authorId, foundAuthor->authorName);
                         } else {
                             printf("Author '%s' not found.\n", authorName);
                         }
                         break;
                     }
                     case 6: break;
                     default: printf("Invalid choice.\n");
                 }
                 break;

            case 3: // Student Operations
                 printf("\n--- Student Operations ---\n");
                 printf("1. Add Student\n");
                 printf("2. Delete Student by ID\n");
                 printf("3. Delete Student by Name\n");
                 printf("4. Update Student\n");
                 printf("5. List All Students\n");
                 printf("6. Find Student by Name\n");
                 printf("7. View Student Info (including loans)\n");
                 printf("8. List Students with Penalty\n");
                 printf("9. Back to Main Menu\n");
                 printf("Enter your choice: ");
                 int studentChoice;
                 scanf("%d", &studentChoice);
                 getchar(); 

                 switch (studentChoice) {
                     case 1: addStudent(&studentHead); break;
                     case 2: deleteStudentById(&studentHead, loanHead); break;
                     case 3: deleteStudentByName(&studentHead, loanHead); break;
                     case 4: updateStudent(studentHead); break;
                     case 5: printStudents(studentHead); break;
                      case 6: {
                         char studentName[MAX_NAME_LEN];
                         printf("Enter Student Name to find: ");
                         fgets(studentName, sizeof(studentName), stdin);
                         studentName[strcspn(studentName, "\n")] = 0;
                         Student *foundStudent = findStudentByName(studentHead, studentName);
                         if (foundStudent) {
                             printf("Student Found: ID %d, Name: %s, Penalty Days: %d\n", foundStudent->studentId, foundStudent->studentName, foundStudent->penaltyDays);
                         } else {
                             printf("Student '%s' not found.\n", studentName);
                         }
                         break;
                     }
                     case 7: printStudentInfo(studentHead, loanHead); break;
                     case 8: printStudentsWithPenalty(studentHead); break;
                     case 9: break; 
                     default: printf("Invalid choice.\n");
                 }
                 break;

            case 4: // Book Loan Operations
                printf("\n--- Book Loan Operations ---\n");
                printf("1. Borrow Book\n");
                printf("2. Return Book\n");
                printf("3. List All Book Loans\n");
                printf("4. List Overdue Loans\n");
                printf("5. Back to Main Menu\n");
                printf("Enter your choice: ");
                int loanChoice;
                scanf("%d", &loanChoice);
                getchar(); 

                switch (loanChoice) {
                    case 1: addBookLoan(&loanHead, bookHead); break;
                    case 2: returnBook(&loanHead, bookHead); break;
                    case 3: printBookLoans(loanHead); break;
                    case 4: printOverdueLoans(loanHead); break;
                    case 5: break; 
                    default: printf("Invalid choice.\n");
                }
                break;

            case 5: // Book-Author Link Operations
                 printf("\n--- Book-Author Link Operations ---\n");
                 printf("1. Add Book-Author Link\n");
                 printf("2. List All Book-Author Links\n");
                 printf("3. Back to Main Menu\n");
                 printf("Enter your choice: ");
                 int bookAuthorChoice;
                 scanf("%d", &bookAuthorChoice);
                 getchar(); 

                 switch (bookAuthorChoice) {
                     case 1: {
                         int bookId, authorId;
                         printf("Enter Book ID: ");
                         scanf("%d", &bookId);
                         getchar();
                         printf("Enter Author ID: ");
                         scanf("%d", &authorId);
                         getchar();
                         addBookAuthor(&bookAuthorArray, &bookAuthorCount, bookId, authorId);
                         break;
                     }
                     case 2: printBookAuthors(bookAuthorArray, bookAuthorCount); break;
                     case 3: break; 
                     default: printf("Invalid choice.\n");
                 }
                 break;


            case 0: // Exit
                printf("Exiting program. Saving data...\n");
                saveBooks(bookHead);
                saveAuthors(authorHead);
                saveStudents(studentHead);
                saveBookLoans(loanHead);
                saveBookAuthors(bookAuthorArray, bookAuthorCount);

                // Free allocated memory
                freeBooks(bookHead);
                freeAuthors(authorHead);
                freeStudents(studentHead);
                freeBookLoans(loanHead);
                free(bookAuthorArray); // Free the dynamic array

                printf("Data saved and memory freed. Goodbye!\n");
                break;

            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);

    return 0;
}

// Print main menu
void printMenu() {
    printf("\n--- Library Management System ---\n");
    printf("1. Book Operations\n");
    printf("2. Author Operations\n");
    printf("3. Student Operations\n");
    printf("4. Book Loan Operations\n");
    printf("5. Book-Author Link Operations\n");
    printf("0. Exit\n");
    printf("----------------------------------\n");
}
