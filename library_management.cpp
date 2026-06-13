/*
 * ================================================================
 *   Library Management System
 *   Language  : C++17
 *   Paradigm  : Object-Oriented Programming + File Handling
 *   Storage   : books.dat | members.dat | borrow.dat (binary)
 *
 *   Features:
 *     • Add / Display / Search / Update / Delete Books
 *     • Add / Display / Search Members
 *     • Issue & Return Books with due-date tracking
 *     • Fine calculation for overdue returns
 *     • Borrowing history per member
 *     • Search by Title or Author
 *     • Admin statistics dashboard
 * ================================================================
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <ctime>
#include <sstream>
#include <cstring>

using namespace std;

// ────────────────────────────────────────────
//  Constants
// ────────────────────────────────────────────
const string BOOKS_FILE   = "books.dat";
const string MEMBERS_FILE = "members.dat";
const string BORROW_FILE  = "borrow.dat";
const int    LOAN_DAYS    = 14;       // 2-week loan period
const double FINE_PER_DAY = 2.0;     // ₹2 per day overdue

// ────────────────────────────────────────────
//  Data Structures
// ────────────────────────────────────────────
struct Book {
    int    bookId;
    char   title[80];
    char   author[60];
    char   publisher[60];
    char   isbn[20];
    char   genre[30];
    int    year;
    int    totalCopies;
    int    availableCopies;
    bool   active;
};

struct Member {
    int  memberId;
    char name[60];
    char email[60];
    char phone[15];
    char address[80];
    char joinDate[25];
    bool active;
    int  booksBorrowed;   // lifetime count
};

struct BorrowRecord {
    int  recordId;
    int  memberId;
    int  bookId;
    char memberName[60];
    char bookTitle[80];
    char issueDate[25];
    char dueDate[25];
    char returnDate[25];  // empty if not returned
    bool returned;
    double fine;
};

// ────────────────────────────────────────────
//  Utility Helpers
// ────────────────────────────────────────────
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void clearIn() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void printLine(char ch = '-', int w = 66) {
    cout << "  " << string(w, ch) << "\n";
}

void printBanner(const string& title) {
    clearScreen();
    printLine('=');
    int pad = (66 - (int)title.size()) / 2;
    cout << "  " << string(max(0,pad), ' ') << title << "\n";
    printLine('=');
    cout << "\n";
}

string currentDateTime() {
    time_t now = time(nullptr);
    char buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

string currentDate() {
    time_t now = time(nullptr);
    char buf[12];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&now));
    return string(buf);
}

string dueDateFromToday() {
    time_t now = time(nullptr);
    now += (time_t)(LOAN_DAYS * 24 * 3600);
    char buf[12];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&now));
    return string(buf);
}

// Days between two "YYYY-MM-DD" strings (d2 - d1)
int daysBetween(const string& d1, const string& d2) {
    struct tm t1{}, t2{};
    sscanf(d1.c_str(), "%d-%d-%d", &t1.tm_year, &t1.tm_mon, &t1.tm_mday);
    sscanf(d2.c_str(), "%d-%d-%d", &t2.tm_year, &t2.tm_mon, &t2.tm_mday);
    t1.tm_year -= 1900; t1.tm_mon -= 1;
    t2.tm_year -= 1900; t2.tm_mon -= 1;
    time_t tt1 = mktime(&t1), tt2 = mktime(&t2);
    return (int)((tt2 - tt1) / 86400);
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ────────────────────────────────────────────
//  File I/O – Generic binary load/save
// ────────────────────────────────────────────
template<typename T>
vector<T> loadFile(const string& fname) {
    vector<T> list;
    ifstream fin(fname, ios::binary);
    if (!fin) return list;
    T item;
    while (fin.read(reinterpret_cast<char*>(&item), sizeof(T)))
        list.push_back(item);
    return list;
}

template<typename T>
bool saveFile(const string& fname, const vector<T>& list) {
    ofstream fout(fname, ios::binary | ios::trunc);
    if (!fout) return false;
    for (auto& item : list)
        fout.write(reinterpret_cast<const char*>(&item), sizeof(T));
    return true;
}

// ────────────────────────────────────────────
//  ID generators
// ────────────────────────────────────────────
int nextBookId() {
    auto list = loadFile<Book>(BOOKS_FILE);
    int mx = 1000;
    for (auto& b : list) mx = max(mx, b.bookId);
    return mx + 1;
}

int nextMemberId() {
    auto list = loadFile<Member>(MEMBERS_FILE);
    int mx = 2000;
    for (auto& m : list) mx = max(mx, m.memberId);
    return mx + 1;
}

int nextRecordId() {
    auto list = loadFile<BorrowRecord>(BORROW_FILE);
    int mx = 3000;
    for (auto& r : list) mx = max(mx, r.recordId);
    return mx + 1;
}

// ────────────────────────────────────────────
//  Find helpers
// ────────────────────────────────────────────
int findBook(const vector<Book>& list, int id) {
    for (int i = 0; i < (int)list.size(); i++)
        if (list[i].bookId == id) return i;
    return -1;
}

int findMember(const vector<Member>& list, int id) {
    for (int i = 0; i < (int)list.size(); i++)
        if (list[i].memberId == id) return i;
    return -1;
}

// ────────────────────────────────────────────
//  Print helpers
// ────────────────────────────────────────────
void printBookHeader() {
    cout << "  " << left
         << setw(6)  << "ID"
         << setw(32) << "Title"
         << setw(22) << "Author"
         << setw(6)  << "Avail" << "\n";
    printLine();
}

void printBookRow(const Book& b) {
    cout << "  " << left
         << setw(6)  << b.bookId
         << setw(32) << string(b.title).substr(0,31)
         << setw(22) << string(b.author).substr(0,21)
         << setw(6)  << b.availableCopies
         << (b.availableCopies == 0 ? "[OUT]" : "") << "\n";
}

void printMemberHeader() {
    cout << "  " << left
         << setw(8)  << "ID"
         << setw(24) << "Name"
         << setw(24) << "Email"
         << "Phone\n";
    printLine();
}

void printMemberRow(const Member& m) {
    cout << "  " << left
         << setw(8)  << m.memberId
         << setw(24) << string(m.name).substr(0,23)
         << setw(24) << string(m.email).substr(0,23)
         << m.phone << "\n";
}

// ════════════════════════════════════════════
//  BOOK OPERATIONS
// ════════════════════════════════════════════

void addBook() {
    printBanner("ADD NEW BOOK");
    auto list = loadFile<Book>(BOOKS_FILE);
    Book b{};
    b.bookId = nextBookId();
    b.active = true;

    clearIn();
    cout << "  Book ID (auto): " << b.bookId << "\n\n";
    cout << "  Title      : "; cin.getline(b.title,     80);
    cout << "  Author     : "; cin.getline(b.author,    60);
    cout << "  Publisher  : "; cin.getline(b.publisher, 60);
    cout << "  ISBN       : "; cin.getline(b.isbn,      20);
    cout << "  Genre      : "; cin.getline(b.genre,     30);

    cout << "  Year       : "; cin >> b.year;      clearIn();
    cout << "  Copies     : "; cin >> b.totalCopies; clearIn();
    b.availableCopies = b.totalCopies;

    list.push_back(b);
    saveFile(BOOKS_FILE, list);
    cout << "\n  ✓  Book [" << b.bookId << "] \"" << b.title << "\" added!\n";
    pause();
}

void displayBooks() {
    printBanner("ALL BOOKS");
    auto list = loadFile<Book>(BOOKS_FILE);
    if (list.empty()) { cout << "  No books found.\n"; pause(); return; }

    printBookHeader();
    for (auto& b : list) if (b.active) printBookRow(b);
    printLine();
    cout << "  Total: " << list.size() << " book(s)\n";
    pause();
}

void searchBooks() {
    printBanner("SEARCH BOOKS");
    cout << "  Search by [1] Title  [2] Author  [3] Book ID: ";
    int ch; cin >> ch; clearIn();

    auto list = loadFile<Book>(BOOKS_FILE);
    vector<Book> res;

    if (ch == 1) {
        cout << "  Title (partial): "; string key; getline(cin, key);
        string lk = toLower(key);
        for (auto& b : list)
            if (b.active && toLower(b.title).find(lk) != string::npos) res.push_back(b);
    } else if (ch == 2) {
        cout << "  Author (partial): "; string key; getline(cin, key);
        string lk = toLower(key);
        for (auto& b : list)
            if (b.active && toLower(b.author).find(lk) != string::npos) res.push_back(b);
    } else if (ch == 3) {
        cout << "  Book ID: "; int id; cin >> id; clearIn();
        for (auto& b : list) if (b.bookId == id) res.push_back(b);
    } else {
        cout << "  Invalid.\n"; pause(); return;
    }

    cout << "\n";
    if (res.empty()) { cout << "  No matching books found.\n"; pause(); return; }

    printBookHeader();
    for (auto& b : res) printBookRow(b);
    printLine();
    cout << "  Found: " << res.size() << " book(s)\n";
    pause();
}

void viewBookDetails() {
    printBanner("BOOK DETAILS");
    cout << "  Enter Book ID: "; int id; cin >> id; clearIn();
    auto list = loadFile<Book>(BOOKS_FILE);
    int idx = findBook(list, id);
    if (idx < 0) { cout << "\n  ✗  Book not found.\n"; pause(); return; }
    auto& b = list[idx];
    printLine();
    cout << "  Book ID      : " << b.bookId           << "\n";
    cout << "  Title        : " << b.title             << "\n";
    cout << "  Author       : " << b.author            << "\n";
    cout << "  Publisher    : " << b.publisher         << "\n";
    cout << "  ISBN         : " << b.isbn              << "\n";
    cout << "  Genre        : " << b.genre             << "\n";
    cout << "  Year         : " << b.year              << "\n";
    cout << "  Total Copies : " << b.totalCopies       << "\n";
    cout << "  Available    : " << b.availableCopies   << "\n";
    cout << "  Status       : " << (b.active ? "Active" : "Removed") << "\n";
    printLine();
    pause();
}

void updateBook() {
    printBanner("UPDATE BOOK");
    cout << "  Enter Book ID: "; int id; cin >> id; clearIn();
    auto list = loadFile<Book>(BOOKS_FILE);
    int idx = findBook(list, id);
    if (idx < 0) { cout << "\n  ✗  Book not found.\n"; pause(); return; }
    auto& b = list[idx];

    cout << "  (Leave blank to keep current)\n\n";
    char buf[80];
    cout << "  Title [" << b.title << "]: "; cin.getline(buf,80); if(strlen(buf)) strncpy(b.title,buf,80);
    cout << "  Author [" << b.author << "]: "; cin.getline(buf,60); if(strlen(buf)) strncpy(b.author,buf,60);
    cout << "  Publisher [" << b.publisher << "]: "; cin.getline(buf,60); if(strlen(buf)) strncpy(b.publisher,buf,60);
    cout << "  Genre [" << b.genre << "]: "; cin.getline(buf,30); if(strlen(buf)) strncpy(b.genre,buf,30);

    cout << "  Add more copies (0=no change): "; int extra; cin >> extra; clearIn();
    if (extra > 0) { b.totalCopies += extra; b.availableCopies += extra; }

    saveFile(BOOKS_FILE, list);
    cout << "\n  ✓  Book updated.\n";
    pause();
}

void deleteBook() {
    printBanner("REMOVE BOOK");
    cout << "  Enter Book ID: "; int id; cin >> id; clearIn();
    auto list = loadFile<Book>(BOOKS_FILE);
    int idx = findBook(list, id);
    if (idx < 0) { cout << "\n  ✗  Book not found.\n"; pause(); return; }

    cout << "  \"" << list[idx].title << "\" — Confirm remove? (y/n): ";
    char ch; cin >> ch; clearIn();
    if (ch == 'y' || ch == 'Y') {
        list[idx].active = false;
        saveFile(BOOKS_FILE, list);
        cout << "\n  ✓  Book removed from catalogue.\n";
    } else cout << "  Cancelled.\n";
    pause();
}

// ════════════════════════════════════════════
//  MEMBER OPERATIONS
// ════════════════════════════════════════════

void addMember() {
    printBanner("REGISTER NEW MEMBER");
    auto list = loadFile<Member>(MEMBERS_FILE);
    Member m{};
    m.memberId     = nextMemberId();
    m.active       = true;
    m.booksBorrowed = 0;
    strncpy(m.joinDate, currentDate().c_str(), 24);

    clearIn();
    cout << "  Member ID (auto): " << m.memberId << "\n\n";
    cout << "  Name    : "; cin.getline(m.name,    60);
    cout << "  Email   : "; cin.getline(m.email,   60);
    cout << "  Phone   : "; cin.getline(m.phone,   15);
    cout << "  Address : "; cin.getline(m.address, 80);

    list.push_back(m);
    saveFile(MEMBERS_FILE, list);
    cout << "\n  ✓  Member [" << m.memberId << "] \"" << m.name << "\" registered!\n";
    pause();
}

void displayMembers() {
    printBanner("ALL MEMBERS");
    auto list = loadFile<Member>(MEMBERS_FILE);
    if (list.empty()) { cout << "  No members found.\n"; pause(); return; }
    printMemberHeader();
    for (auto& m : list) if (m.active) printMemberRow(m);
    printLine();
    cout << "  Total: " << list.size() << " member(s)\n";
    pause();
}

void searchMember() {
    printBanner("SEARCH MEMBER");
    cout << "  Search by [1] Member ID  [2] Name: ";
    int ch; cin >> ch; clearIn();

    auto list = loadFile<Member>(MEMBERS_FILE);
    vector<Member> res;

    if (ch == 1) {
        cout << "  Member ID: "; int id; cin >> id; clearIn();
        for (auto& m : list) if (m.memberId == id) res.push_back(m);
    } else {
        cout << "  Name (partial): "; string key; getline(cin, key);
        string lk = toLower(key);
        for (auto& m : list)
            if (m.active && toLower(m.name).find(lk) != string::npos) res.push_back(m);
    }

    if (res.empty()) { cout << "\n  No matching members.\n"; pause(); return; }
    cout << "\n";
    printMemberHeader();
    for (auto& m : res) printMemberRow(m);
    printLine();
    pause();
}

void viewMemberProfile() {
    printBanner("MEMBER PROFILE");
    cout << "  Member ID: "; int id; cin >> id; clearIn();
    auto mlist = loadFile<Member>(MEMBERS_FILE);
    int idx = findMember(mlist, id);
    if (idx < 0) { cout << "\n  ✗  Member not found.\n"; pause(); return; }
    auto& m = mlist[idx];

    printLine();
    cout << "  Member ID      : " << m.memberId      << "\n";
    cout << "  Name           : " << m.name           << "\n";
    cout << "  Email          : " << m.email          << "\n";
    cout << "  Phone          : " << m.phone          << "\n";
    cout << "  Address        : " << m.address        << "\n";
    cout << "  Joined         : " << m.joinDate       << "\n";
    cout << "  Books Borrowed : " << m.booksBorrowed  << " (lifetime)\n";
    cout << "  Status         : " << (m.active ? "Active" : "Inactive") << "\n";
    printLine();

    // Show current active borrows
    auto brlist = loadFile<BorrowRecord>(BORROW_FILE);
    cout << "  Currently Borrowed:\n";
    bool any = false;
    for (auto& r : brlist) {
        if (r.memberId == id && !r.returned) {
            cout << "    • [" << r.bookId << "] " << r.bookTitle
                 << "  Due: " << r.dueDate << "\n";
            any = true;
        }
    }
    if (!any) cout << "    None\n";
    printLine();
    pause();
}

// ════════════════════════════════════════════
//  BORROW / RETURN
// ════════════════════════════════════════════

void issueBook() {
    printBanner("ISSUE BOOK");

    cout << "  Member ID : "; int mid; cin >> mid; clearIn();
    auto mlist = loadFile<Member>(MEMBERS_FILE);
    int mi = findMember(mlist, mid);
    if (mi < 0 || !mlist[mi].active) {
        cout << "\n  ✗  Member not found or inactive.\n"; pause(); return;
    }

    // Check if member already has 3 books
    auto brlist = loadFile<BorrowRecord>(BORROW_FILE);
    int active = 0;
    for (auto& r : brlist) if (r.memberId == mid && !r.returned) active++;
    if (active >= 3) {
        cout << "\n  ✗  Member already has 3 books issued (maximum).\n"; pause(); return;
    }

    cout << "  Book ID   : "; int bid; cin >> bid; clearIn();
    auto blist = loadFile<Book>(BOOKS_FILE);
    int bi = findBook(blist, bid);
    if (bi < 0 || !blist[bi].active) {
        cout << "\n  ✗  Book not found.\n"; pause(); return;
    }
    if (blist[bi].availableCopies <= 0) {
        cout << "\n  ✗  No copies available. All copies are issued.\n"; pause(); return;
    }

    // Check not already borrowed same book
    for (auto& r : brlist)
        if (r.memberId == mid && r.bookId == bid && !r.returned) {
            cout << "\n  ✗  Member already has this book.\n"; pause(); return;
        }

    // Create record
    BorrowRecord rec{};
    rec.recordId = nextRecordId();
    rec.memberId = mid;
    rec.bookId   = bid;
    rec.returned = false;
    rec.fine     = 0.0;
    strncpy(rec.memberName, mlist[mi].name,     59);
    strncpy(rec.bookTitle,  blist[bi].title,     79);
    strncpy(rec.issueDate,  currentDate().c_str(), 24);
    strncpy(rec.dueDate,    dueDateFromToday().c_str(), 24);
    memset(rec.returnDate, 0, sizeof(rec.returnDate));

    blist[bi].availableCopies--;
    mlist[mi].booksBorrowed++;

    brlist.push_back(rec);
    saveFile(BOOKS_FILE,   blist);
    saveFile(MEMBERS_FILE, mlist);
    saveFile(BORROW_FILE,  brlist);

    cout << "\n  ✓  Book Issued Successfully!\n";
    printLine();
    cout << "  Record ID  : " << rec.recordId       << "\n";
    cout << "  Member     : " << rec.memberName      << "\n";
    cout << "  Book       : " << rec.bookTitle       << "\n";
    cout << "  Issue Date : " << rec.issueDate       << "\n";
    cout << "  Due Date   : " << rec.dueDate         << "\n";
    cout << "  Loan Period: " << LOAN_DAYS << " days\n";
    printLine();
    pause();
}

void returnBook() {
    printBanner("RETURN BOOK");

    cout << "  Member ID : "; int mid; cin >> mid; clearIn();
    cout << "  Book ID   : "; int bid; cin >> bid; clearIn();

    auto brlist = loadFile<BorrowRecord>(BORROW_FILE);
    int ri = -1;
    for (int i = 0; i < (int)brlist.size(); i++)
        if (brlist[i].memberId == mid && brlist[i].bookId == bid && !brlist[i].returned) {
            ri = i; break;
        }

    if (ri < 0) {
        cout << "\n  ✗  No active borrow record found.\n"; pause(); return;
    }

    string today = currentDate();
    strncpy(brlist[ri].returnDate, today.c_str(), 24);
    brlist[ri].returned = true;

    // Fine calculation
    int overdue = daysBetween(string(brlist[ri].dueDate), today);
    double fine = 0.0;
    if (overdue > 0) {
        fine = overdue * FINE_PER_DAY;
        brlist[ri].fine = fine;
    }

    // Update book availability
    auto blist = loadFile<Book>(BOOKS_FILE);
    int bi = findBook(blist, bid);
    if (bi >= 0) blist[bi].availableCopies++;

    saveFile(BORROW_FILE, brlist);
    saveFile(BOOKS_FILE,  blist);

    cout << "\n  ✓  Book Returned Successfully!\n";
    printLine();
    cout << "  Book       : " << brlist[ri].bookTitle  << "\n";
    cout << "  Due Date   : " << brlist[ri].dueDate    << "\n";
    cout << "  Return Date: " << today                 << "\n";
    if (fine > 0.0)
        cout << "  ⚠  Overdue by " << overdue << " day(s). Fine: ₹"
             << fixed << setprecision(2) << fine << "\n";
    else
        cout << "  No fine. Returned on time.\n";
    printLine();
    pause();
}

void viewBorrowRecords() {
    printBanner("BORROW RECORDS");
    cout << "  [1] All Active  [2] All Records  [3] By Member  [4] Overdue: ";
    int ch; cin >> ch; clearIn();

    auto brlist = loadFile<BorrowRecord>(BORROW_FILE);

    auto printHeader = [&]() {
        cout << "  " << left
             << setw(7)  << "RecID"
             << setw(8)  << "MemID"
             << setw(22) << "Member"
             << setw(6)  << "BkID"
             << setw(20) << "Book"
             << setw(12) << "Due"
             << "Status\n";
        printLine();
    };

    auto printRow = [&](const BorrowRecord& r) {
        string status = r.returned ? "Returned" : "Active";
        string today = currentDate();
        if (!r.returned && daysBetween(string(r.dueDate), today) > 0)
            status = "OVERDUE";
        cout << "  " << left
             << setw(7)  << r.recordId
             << setw(8)  << r.memberId
             << setw(22) << string(r.memberName).substr(0,21)
             << setw(6)  << r.bookId
             << setw(20) << string(r.bookTitle).substr(0,19)
             << setw(12) << r.dueDate
             << status   << "\n";
    };

    cout << "\n";
    printHeader();

    if (ch == 1) {
        for (auto& r : brlist) if (!r.returned) printRow(r);
    } else if (ch == 2) {
        for (auto& r : brlist) printRow(r);
    } else if (ch == 3) {
        cout << "\n  Member ID: "; int mid; cin >> mid; clearIn();
        for (auto& r : brlist) if (r.memberId == mid) printRow(r);
    } else if (ch == 4) {
        string today = currentDate();
        for (auto& r : brlist)
            if (!r.returned && daysBetween(string(r.dueDate), today) > 0) printRow(r);
    }
    printLine();
    pause();
}

// ════════════════════════════════════════════
//  STATISTICS
// ════════════════════════════════════════════
void statistics() {
    printBanner("LIBRARY STATISTICS");
    auto blist  = loadFile<Book>(BOOKS_FILE);
    auto mlist  = loadFile<Member>(MEMBERS_FILE);
    auto brlist = loadFile<BorrowRecord>(BORROW_FILE);

    int totalBooks = 0, availBooks = 0, issuedBooks = 0;
    for (auto& b : blist) {
        if (!b.active) continue;
        totalBooks    += b.totalCopies;
        availBooks    += b.availableCopies;
        issuedBooks   += (b.totalCopies - b.availableCopies);
    }

    int activeMembers = 0;
    for (auto& m : mlist) if (m.active) activeMembers++;

    int activeLoans = 0, overdue = 0;
    double totalFines = 0;
    string today = currentDate();
    for (auto& r : brlist) {
        if (!r.returned) {
            activeLoans++;
            if (daysBetween(string(r.dueDate), today) > 0) overdue++;
        }
        totalFines += r.fine;
    }

    printLine();
    cout << "  📚 Books\n";
    cout << "     Total Titles    : " << blist.size()  << "\n";
    cout << "     Total Copies    : " << totalBooks     << "\n";
    cout << "     Available       : " << availBooks     << "\n";
    cout << "     Currently Issued: " << issuedBooks    << "\n";
    printLine();
    cout << "  👤 Members\n";
    cout << "     Total Members   : " << mlist.size()   << "\n";
    cout << "     Active Members  : " << activeMembers   << "\n";
    printLine();
    cout << "  📋 Transactions\n";
    cout << "     Total Records   : " << brlist.size()  << "\n";
    cout << "     Active Loans    : " << activeLoans     << "\n";
    cout << "     Overdue         : " << overdue         << "\n";
    cout << "     Total Fines     : ₹" << fixed << setprecision(2) << totalFines << "\n";
    printLine();
    pause();
}

// ════════════════════════════════════════════
//  MENUS
// ════════════════════════════════════════════

void bookMenu() {
    int ch;
    do {
        printBanner("BOOK MANAGEMENT");
        cout << "  [1]  Add New Book\n";
        cout << "  [2]  Display All Books\n";
        cout << "  [3]  Search Book (Title / Author / ID)\n";
        cout << "  [4]  View Book Details\n";
        cout << "  [5]  Update Book\n";
        cout << "  [6]  Remove Book\n";
        cout << "  [0]  Back\n";
        printLine();
        cout << "  Choice: ";
        if (!(cin >> ch)) { clearIn(); ch = -1; }
        switch (ch) {
            case 1: addBook();        break;
            case 2: displayBooks();   break;
            case 3: searchBooks();    break;
            case 4: viewBookDetails();break;
            case 5: updateBook();     break;
            case 6: deleteBook();     break;
            case 0: break;
            default: cout << "\n  Invalid.\n"; pause();
        }
    } while (ch != 0);
}

void memberMenu() {
    int ch;
    do {
        printBanner("MEMBER MANAGEMENT");
        cout << "  [1]  Register New Member\n";
        cout << "  [2]  Display All Members\n";
        cout << "  [3]  Search Member\n";
        cout << "  [4]  View Member Profile\n";
        cout << "  [0]  Back\n";
        printLine();
        cout << "  Choice: ";
        if (!(cin >> ch)) { clearIn(); ch = -1; }
        switch (ch) {
            case 1: addMember();        break;
            case 2: displayMembers();   break;
            case 3: searchMember();     break;
            case 4: viewMemberProfile();break;
            case 0: break;
            default: cout << "\n  Invalid.\n"; pause();
        }
    } while (ch != 0);
}

void borrowMenu() {
    int ch;
    do {
        printBanner("BORROW & RETURN");
        cout << "  [1]  Issue Book to Member\n";
        cout << "  [2]  Return Book\n";
        cout << "  [3]  View Borrow Records\n";
        cout << "  [0]  Back\n";
        printLine();
        cout << "  Choice: ";
        if (!(cin >> ch)) { clearIn(); ch = -1; }
        switch (ch) {
            case 1: issueBook();         break;
            case 2: returnBook();        break;
            case 3: viewBorrowRecords(); break;
            case 0: break;
            default: cout << "\n  Invalid.\n"; pause();
        }
    } while (ch != 0);
}

int main() {
    int ch;
    do {
        printBanner("LIBRARY MANAGEMENT SYSTEM");
        cout << "  [1]  Book Management\n";
        cout << "  [2]  Member Management\n";
        cout << "  [3]  Issue / Return Books\n";
        cout << "  [4]  Library Statistics\n";
        cout << "  [0]  Exit\n";
        printLine();
        cout << "  Choice: ";
        if (!(cin >> ch)) { clearIn(); ch = -1; }
        switch (ch) {
            case 1: bookMenu();   break;
            case 2: memberMenu(); break;
            case 3: borrowMenu(); break;
            case 4: statistics(); break;
            case 0: break;
            default: cout << "\n  Invalid option.\n"; pause();
        }
    } while (ch != 0);

    clearScreen();
    cout << "\n  Thank you for using the Library Management System!\n\n";
    return 0;
}
