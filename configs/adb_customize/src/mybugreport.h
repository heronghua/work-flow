#ifndef MYBUGREPORT_H
#define MYBUGREPORT_H

#include <string>
#include "line_printer.h"

class MyBugreport {
public:
    MyBugreport() : line_printer_() {}
    int DoIt(int argc, const char** argv);

private:
    int RunScreenRecord(const std::string& output_path);
    void WaitForEKey();

    LinePrinter line_printer_;
};

#endif // MYBUGREPORT_H
