#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <cstdio>
char readKey() {
    if (!_isatty(_fileno(stdin))) {
        char c;
        if (std::cin >> c)
            return c;
        return 'q';
    }
    int c = _getch();
    if (c == 0 || c == 224) {
        switch (_getch()) {
            case 72: return 'i';
            case 80: return 'k';
            case 75: return 'j';
            case 77: return 'l';
        }
        return '\0';
    }
    return static_cast<char>(c);
}
#else
#include <termios.h>
#include <unistd.h>

char readKey() {
    if (!isatty(STDIN_FILENO)) {
        char c;
        if (std::cin >> c)
            return c;
        return 'q';
    }

    termios old{};
    tcgetattr(STDIN_FILENO, &old);
    termios raw = old;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        c = 'q';
    } else if (c == 27) {
        char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
            read(STDIN_FILENO, &seq[1], 1) == 1 && seq[0] == '[') {
            switch (seq[1]) {
                case 'A': c = 'i'; break;
                case 'B': c = 'k'; break;
                case 'C': c = 'l'; break;
                case 'D': c = 'j'; break;
            }
            }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return c;
}
#endif

class Position {
    int x, y;
public:
    explicit Position(int x = 0, int y = 0)
        :x(x), y(y){}

    friend std::ostream& operator<<(std::ostream& os, const Position& position){
        os << position.x << " " << position.y;
        return os;
    }

    int getX() const{
        return x;
    }

    int getY() const{
        return y;
    }

    void move(int dx, int dy) {
        x += dx;
        y += dy;
    }
};

class Character {
    char type;
    Position position;
    bool isAlive;
public:
    Character(char type, const Position& position, bool isAlive = true)
        :type(type), position(position), isAlive(isAlive){}

    friend std::ostream& operator<<(std::ostream& os, const Character& c) {
        os << c.type << " " << c.position << " " << c.isAlive;
        return os;
    }

    const Position& getPosition() const {
        return position;
    }

    char getType() const{
        return type;
    }

    bool getAlive() const {
        return isAlive;
    }

    void move(int dx, int dy) {
        position.move(dx,dy);
    }

    void die() {
        isAlive = false;
    }
};

class Door {
    char type;
    Position position;

    bool goodPosition(const Character& c) const{
        return ((c.getPosition().getX() == position.getX()) && (c.getPosition().getY() == position.getY()));
    }

    bool goodType(const Character& c) const {
        if (c.getType() == 'F' && type == '%')
            return true;
        else if (c.getType() == 'W' && type == '&')
            return true;
        return false;
    }

public:
    Door(char typeChar, const Position& position)
        :type(typeChar), position(position){}

    friend std::ostream& operator<<(std::ostream& os, const Door& door) {
        os << door.type << " " << door.position;
        return os;
    }

    bool canUse(const Character& c) const {
        // verific daca pozitia este buna
        return (goodPosition(c) && goodType(c) && c.getAlive());
    }

};

class Tile {
    char symbol;
public:
    explicit Tile(char symbol = '.')
        :symbol(symbol){}

    friend std::ostream& operator<<(std::ostream& os, const Tile& tile) {
        os << tile.symbol;
        return os;
    }

    // vreau sa verific ce fel de casuta e
    bool isWall() const{
        return (symbol == 'w');
    }

    bool isWater() const{
        return (symbol == '~');
    }

    bool isFire() const{
        return (symbol == '*');
    }

    bool isDiamond() const{
        return (symbol == '+' || symbol == '-');
    }

    bool isDiamondF() const {
        return (symbol == '+');
    }

    bool isDiamondW() const {
        return (symbol == '-');
    }

    bool isMud() const {
        return (symbol == '_');
    }

    bool isButton() const {
        return (symbol == 'o');
    }

    bool isGate() const {
        return (symbol == 'I');
    }

    bool isCFire() const {
        return (symbol == 'F');
    }

    bool isCWater() const {
        return (symbol == 'W');
    }

    bool isDFire() const {
        return (symbol == '%');
    }

    bool isDWater() const {
        return (symbol == '&');
    }

    bool deadlyFor(const Character& c) const {
        // daca este noroi, moare
        if (isMud())
            return true;
        if (isWater() && (c.getType()=='F'))
            return true;
        if (isFire() && (c.getType()=='W'))
            return true;

        return false;
    }
};

class Level {

    int h;
    int w;
    Tile* grid;
    Position cfire, cwater, dfire, dwater, gate, button1, button2;

    int index(int x, int y) const {
        return y*w+x;
    }

    void find() {
        bool CFire = false, DFire = false, CWater = false, DWater = false, Gate = false, Button1 = false, Button2 = false;
        for (int y=0 ; y<h; y++)
            for (int x=0;x<w;x++) {
                if (grid[index(x,y)].isCFire()) {
                    cfire = Position(x,y);
                    CFire = true;
                    emptyTile(x,y);
                }else if (grid[index(x,y)].isDFire()) {
                    DFire = true;
                    dfire = Position(x,y);
                }else if (grid[index(x,y)].isCWater()) {
                    cwater = Position(x,y);
                    CWater= true;
                    emptyTile(x,y);
                }else if (grid[index(x,y)].isDWater()) {
                    dwater = Position(x,y);
                    DWater= true;
                }else if (grid[index(x,y)].isButton() && Button1 == false) {
                    button1 = Position(x,y);
                    Button1 = true;
                }else if (grid[index(x,y)].isButton()) {
                    button2 = Position(x,y);
                    Button2 = true;
                }else if (grid[index(x,y)].isGate()) {
                    gate = Position(x,y);
                    Gate = true;
                }
            }

        if (CFire == false || CWater == false) {
            delete[] grid;
            throw std::runtime_error("A character is missing from the map.");
        }

        if (DFire == false || DWater == false) {
            delete[] grid;
            throw std::runtime_error("A door is missing from the map.");
        }

        if (Button1 == false || Button2 == false || Gate == false) {
            delete[] grid;
            throw std::runtime_error("Gate or button missing from the map");
        }
    }
public:
    explicit Level(const std::string& fileName)
        :h(0), w(0), grid(nullptr) {

        std::ifstream file(fileName);
        if (!file) {
            throw std::runtime_error("Unable to open file");
        }
        file >> h >> w;
        if (h <= 0 || w <= 0 || !file)
            throw std::runtime_error("Error reading h and w.");
        grid = new Tile[h*w];

        char ch;

        for (int y=0; y<h; y++) {
            for (int x=0; x<w; x++) {
                if (!file.get(ch)) {
                    delete[] grid;
                    throw std::runtime_error("Not enough characters!");
                }

                if (ch == '\r' || ch == '\n'){
                    x--;
                    continue;
                }
                grid[index(x,y)] = Tile(ch);
            }
        }
        find();
    }

    friend std::ostream& operator<<(std::ostream& os, const Level& level) {
        os << level.h << " " << level.w << std::endl;

        for (int y=0; y<level.h; y++) {
            for (int x=0; x<level.w; x++) {
                os << level.grid[level.index(x,y)];
            }
            os << "\n";
        }
        return os;
    }

    Level(const Level& other)
        :h(other.h), w(other.w), grid(new Tile[h*w]), cfire(other.cfire), cwater(other.cwater), dfire(other.dfire), dwater(other.dwater), gate(other.gate), button1(other.button1), button2(other.button2) {

        for (int y=0; y<h; y++) {
            for (int x=0; x<w; x++) {
                grid[index(x,y)] = other.grid[index(x,y)];
            }
        }
    }

    Level& operator=(const Level& other) {
        if (this != &other) {
            h = other.h;
            w = other.w;
            cfire = other.cfire;
            cwater = other.cwater;
            dfire = other.dfire;
            dwater = other.dwater;
            gate = other.gate;
            button1 = other.button1;
            button2 = other.button2;
            delete[] grid;
            grid = new Tile[h*w];

            for (int y=0; y<h; y++) {
                for (int x=0; x<w; x++) {
                    grid[index(x,y)] = other.grid[index(x,y)];
                }
            }
        }

        return *this;
    }

    void setTile(int x, int y, const Tile &tile) {
        grid[index(x, y)] = tile; //getTile eliminat
    }

    // ca sa vad ce fel de tile este trebuie sa accesez la pozitia x si y simbolul tile
    bool inMap(int x, int y) const {
        return (y >= 0 && y < h && x >= 0 && x < w);
    }

    const Tile &tileAt(int x, int y) const {
        return grid[index(x, y)];
    }

    void emptyTile(int x, int y) {
        grid[index(x,y)] = Tile('.');
    }

    bool hasDiamonds() const {
        for (int y=0 ; y<h; y++)
            for (int x=0;x<w;x++) {
                if (grid[index(x,y)].isDiamond())
                    return true;
            }
        return false;
    }

    const Position& startCFire() const {
        return cfire;
    }

    const Position& startCWater() const {
        return cwater;
    }

    const Position& startDFire() const {
        return dfire;
    }

    const Position& startDWater() const {
        return dwater;
    }

    bool verifiedWall() const{
        for (int y=0 ; y<h; y++)
            for (int x=0;x<w;x++) {
                if (x==0 || x==w-1 || y==0 || y==h-1)
                    if (!(grid[index(x,y)].isWall()))
                        return false;
            }

        return true;
    }

    bool pressedButton1(const Character& c) const{
        return ((button1.getX() == c.getPosition().getX()) && (button1.getY() == c.getPosition().getY()));
    }

    bool pressedButton2(const Character& c) const{
        return ((button2.getX() == c.getPosition().getX()) && (button2.getY() == c.getPosition().getY()));
    }

    ~Level() {
        delete[] grid;
    }
};
class Game {
    Level level;
    Character fire;
    Character water;
    Door fdoor;
    Door wdoor;
    int diamonds;

    Character* characterFor(char key) {
        if (key == 'w' || key == 'a' || key == 's' || key == 'd') return &fire;
        if (key == 'i' || key == 'j' || key == 'k' || key == 'l') return &water;
        return nullptr;
    }
    static void directionFor(char key, int& dx, int& dy) {
        dx = 0;
        dy = 0;
        if (key=='w' || key=='i'){ dx = 0; dy = -1;}
        if (key=='a' || key=='j'){ dx = -1; dy = 0;}
        if (key=='s' || key=='k'){ dx = 0; dy = 1;}
        if (key=='d' || key=='l'){ dx = 1; dy = 0;}
    }
public:
    explicit Game(const Level& level, int diamonds = 0)
        :level(level), fire('F', level.startCFire()), water('W', level.startCWater()), fdoor('%', level.startDFire()), wdoor('&', level.startDWater()), diamonds(diamonds){}

    friend std::ostream& operator<<(std::ostream& os, const Game& game) {
        Level view = game.level;
        view.setTile(game.fire.getPosition().getX(), game.fire.getPosition().getY(), Tile('F'));
        view.setTile(game.water.getPosition().getX(), game.water.getPosition().getY(), Tile('W'));

        os << view;
        return os;
    }

    bool won() const {
        return (fdoor.canUse(fire) && wdoor.canUse(water) && !level.hasDiamonds());
    }

    bool lost() const {
        return !(fire.getAlive() && water.getAlive());
    }

    void move(char key) {
        // if (level.verifiedWall() && fire.getAlive() && water.getAlive()) {
        // }
        //verificare tile daca este deadly atunci moare

        Character* who = characterFor(key);
        if (who == nullptr || !who->getAlive())
            return;

        int dx, dy, nx, ny;
        directionFor(key, dx, dy);

        nx = who->getPosition().getX() + dx;
        ny = who->getPosition().getY() + dy;

        if (!level.inMap(nx, ny))
            return;

        if (level.tileAt(nx,ny).isWall())
            return;
// am in fata o poarta si niciun buton nu e apasat
        if (level.tileAt(nx,ny).isGate() && !(level.pressedButton1(who == &fire ? water : fire) || level.pressedButton2(who == &fire ? water : fire)))
            return;

        who->move(dx,dy);

        if (level.tileAt(nx, ny).deadlyFor(*who)) {
            who->die();
        }else if (level.tileAt(nx,ny).isDiamond()) {
            if ((level.tileAt(nx,ny).isDiamondW() && who->getType() == 'W') || (level.tileAt(nx,ny).isDiamondF() && who->getType() == 'F')) {
                diamonds++;
                level.emptyTile(nx,ny);
            }
        }
    }
};
int main() {

    try {
    std::cout << "=== Position ===\n";
    Position p(3, 4);
    std::cout << "p = " << p << "\n";
    std::cout << "x = " << p.getX() << ", y = " << p.getY() << "\n";
    p.move(1, 0);
    std::cout << "dupa move(1, 0): " << p << "\n";

    std::cout << "\n=== Character ===\n";
    Character c('F', p);
    std::cout << "c = " << c << "\n";
    std::cout << "pozitie: " << c.getPosition() << ", tip: " << c.getType()
                  << ", viu: " << c.getAlive() << "\n";
    c.move(1, 0);
    std::cout << "dupa move(1, 0): " << c << "\n";
    c.die();
    std::cout << "dupa die(): " << c << "\n";

    std::cout << "\n=== Door ===\n";
    Door df('%', Position(5, 4)), dw('&', Position(5, 4));
    std::cout << "df = " << df << ", dw = " << dw << "\n";
    Character c2('F', Position(5, 4));
    std::cout << "c2 (F, viu) pe usa de foc: " << df.canUse(c2) << "\n";
    std::cout << "c2 (F, viu) pe usa de apa: " << dw.canUse(c2) << "\n";
    std::cout << "c (F, mort) pe usa de foc: " << df.canUse(c) << "\n";

    std::cout << "\n=== Tile ===\n";
    Tile apa('~'), foc('*'), noroi('_'), gol('.'), zid('w'), diamantf('+'), diamantw('-');
    Tile tf('F'), tw('W'), uf('%'), uw('&');
    std::cout << "apa = " << apa << "\n";
    std::cout << "isWall: " << zid.isWall() << ", isWater: " << apa.isWater()
        << ", isFire: " << foc.isFire() << ", isDiamond: " << diamantf.isDiamond() << ", isDiamondF: " << diamantf.isDiamondF() << ", isDiamondW: " << diamantw.isDiamondW()
        << ", isMud: " << noroi.isMud() << "\n";
    std::cout << "isCFire: " << tf.isCFire() << ", isCWater: " << tw.isCWater()
        << ", isDFire: " << uf.isDFire() << ", isDWater: " << uw.isDWater() << "\n";
    Character f('F', Position(1, 1)), wat('W', Position(1, 1));
    std::cout << "apa mortala pentru F/W: " << apa.deadlyFor(f) << apa.deadlyFor(wat) << "\n";
    std::cout << "foc mortal pentru F/W: " << foc.deadlyFor(f) << foc.deadlyFor(wat) << "\n";
    std::cout << "noroi mortal pentru F/W: " << noroi.deadlyFor(f) << noroi.deadlyFor(wat) << "\n";
    std::cout << "gol mortal pentru F/W: " << gol.deadlyFor(f) << gol.deadlyFor(wat) << "\n";

    std::cout << "\n=== Level ===\n";
    Level l("assets/map.txt");
    std::cout << l;
    std::cout << "margini ziduri: " << l.verifiedWall() << "\n";
    std::cout << "inMap(0, 0): " << l.inMap(0, 0) << ", inMap(-1, 0): " << l.inMap(-1, 0) << "\n";
    std::cout << "tileAt(0, 0): " << l.tileAt(0, 0) << "\n";
    std::cout << "start foc: " << l.startCFire() << ", usa foc: " << l.startDFire() << "\n";
    std::cout << "start apa: " << l.startCWater() << ", usa apa: " << l.startDWater() << "\n";
    std::cout << "are diamante: " << l.hasDiamonds() << "\n";

        Tile buton('o'), poarta('I');
        std::cout << "isButton: " << buton.isButton() << ", isGate: " << poarta.isGate() << "\n";

    Level a = l;
    a.setTile(1, 1, Tile('+'));
    std::cout << "copia: tileAt(1, 1) = " << a.tileAt(1, 1)
        << ", hasDiamonds = " << a.hasDiamonds() << "\n";
    std::cout << "originalul: tileAt(1, 1) = " << l.tileAt(1, 1)
        << ", hasDiamonds = " << l.hasDiamonds() << "\n";
    a.emptyTile(1, 1);
    std::cout << "dupa emptyTile in copie: " << a.tileAt(1, 1) << "\n";

    Level b("assets/map.txt");
    b = a;
    b.setTile(1, 1, Tile('~'));
    std::cout << "dupa operator=: b = " << b.tileAt(1, 1) << ", a = " << a.tileAt(1, 1) << "\n";

    std::cout << "\n=== Game ===\n";
    Game demo(l);
    std::cout << "won: " << demo.won() << ", lost: " << demo.lost() << "\n";
    demo.move('d');
    std::cout << demo;

    std::cout << "\n=== Joc ===\n";
        Level le("assets/map.txt");

        if (!le.verifiedWall())
            throw std::runtime_error("Walls missing.");
        std::cout << le;

        Game game(le);
        char key = ' ';

    while (!game.won() && !game.lost() && key != 'q') {
        std::cout << game;
        key = readKey();

        if (key == 'q')
            break;

        game.move(key);
    }

        std::cout << game;
    if (game.won())
        std::cout << "You won!\n";
    else if (game.lost())
        std::cout << "Game over.\n";
    else
        std::cout << "You left the game.\n";
    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
        return 1;
    }
    // w - wall
    // . - empty
    // d - diamond
    // %  - door fire
    // & - door water
    // o - button
    // I - gate
    // ~ - water
    // * - fire
    // _ - mud
    // F - fire character
    // W - water character
    return 0;
}