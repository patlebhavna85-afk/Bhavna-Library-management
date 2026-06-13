# 📚 Library Management System (C++)

A complete, console-based **Library Management System** built in **C++17** using **Object-Oriented Programming** and **File Handling**. Efficiently manages books, members, and borrowing records.

---

## ✨ Key Features

### 📖 Book Management
| Feature | Description |
|---------|-------------|
| Add Book | Title, Author, Publisher, ISBN, Genre, Year, Copies |
| Display All | Full catalogue with availability status |
| Search | By **Title** (partial), **Author** (partial), or Book ID |
| View Details | Complete book profile |
| Update | Edit any field, add more copies |
| Remove | Soft-delete from catalogue |

### 👤 Member Management
| Feature | Description |
|---------|-------------|
| Register Member | Auto-generated Member ID |
| Display All | List of all active members |
| Search | By Member ID or partial name |
| View Profile | Details + currently borrowed books |

### 🔄 Borrow & Return
| Feature | Description |
|---------|-------------|
| Issue Book | Validates availability, member limit (max 3), duplicates |
| Return Book | Marks return, auto-restores copy count |
| Fine Calculation | ₹2/day for overdue returns |
| Borrow Records | Filter: Active / All / By Member / Overdue |

### 📊 Statistics Dashboard
- Total books, copies, available vs issued
- Total & active members
- Active loans, overdue count
- Total fines collected

---

## 🗂️ File Structure

```
library-management/
├── library_management.cpp    # All source code (~550 lines)
├── books.dat                 # Auto-created binary (book records)
├── members.dat               # Auto-created binary (member records)
├── borrow.dat                # Auto-created binary (borrow records)
└── README.md
```

---

## 🏗️ OOP Design

```
Structs / Data Models
├── Book          → bookId, title, author, publisher, isbn, genre,
│                   year, totalCopies, availableCopies, active
├── Member        → memberId, name, email, phone, address,
│                   joinDate, active, booksBorrowed
└── BorrowRecord  → recordId, memberId, bookId, memberName, bookTitle,
                    issueDate, dueDate, returnDate, returned, fine

Template Functions
├── loadFile<T>()   → Generic binary file loader
└── saveFile<T>()   → Generic binary file saver

Modules
├── Book Ops      → addBook, displayBooks, searchBooks, updateBook, deleteBook
├── Member Ops    → addMember, displayMembers, searchMember, viewMemberProfile
└── Borrow Ops    → issueBook, returnBook, viewBorrowRecords
```

---

## 📋 Business Rules

| Rule | Value |
|------|-------|
| Loan Period | 14 days |
| Max books per member | 3 |
| Fine for overdue | ₹2 per day |
| Minimum deposit for FD | ₹1,000 |

---

## 🛠️ Build & Run

### Linux / macOS
```bash
g++ -std=c++17 -o lms library_management.cpp
./lms
```

### Windows (MinGW)
```bash
g++ -std=c++17 -o lms.exe library_management.cpp
lms.exe
```

### Windows (MSVC)
```bash
cl /EHsc /std:c++17 library_management.cpp /Fe:lms.exe
lms.exe
```

---

## 📸 Menu Preview

```
  ══════════════════════════════════════════════════════════════════
                     LIBRARY MANAGEMENT SYSTEM
  ══════════════════════════════════════════════════════════════════

  [1]  Book Management
  [2]  Member Management
  [3]  Issue / Return Books
  [4]  Library Statistics
  [0]  Exit
```

---

## 👩‍💻 Author

Feel free to ⭐ star, fork, and contribute!
