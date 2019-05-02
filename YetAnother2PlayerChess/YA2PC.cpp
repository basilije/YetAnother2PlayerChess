#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <string.h>
#include <cwchar>
#include <fstream>
#include <ctime>


enum FigureType { KING, QUEEN, BISHOP, KNIGHT, ROOK, PAWN, NO_FIGURE };
enum FigureColor { WHITE, BLACK, NO_COLOR };

struct ChessBoardSquare
{
	FigureType figure_type = NO_FIGURE;
	FigureColor figure_color = NO_COLOR;
};

ChessBoardSquare chessboard_squares[8][8];
FigureType first_row_figure_order[8]{ ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK };
const char* white_n_black_figures = "♔♕♗♘♖♙♚♛♝♞♜♟";
const char* empty_white_n_black_chars = "□■";
const char* columns_abcdefgh = "ⒶⒷⒸⒹⒺⒻⒼⒽ";
const char* rows_12345678 = "①②③④⑤⑥⑦⑧";
const char* bottom_neutral_symbol = "◯";
const char* check_white_symbol = "◎";
const char* check_black_symbol = "⦿";
const char* no_winner = "☮";
const char* white_wins = "☺☺";
const char* black_wins = "☻☻";
const int FIGURE_CHAR_SIZE = 3;
const int EMPTY_CHAR_SIZE = 3;

FigureColor color_to_play = WHITE;
bool white_in_check = false;
bool black_in_check = false;
bool whites_king_castling_figures_untouched = true;
bool whites_queen_castling_figures_untouched = true;
bool blacks_king_castling_figures_untouched = true;
bool blacks_queen_castling_figures_untouched = true;
COORD homeCoords = { 0, 0 };
HANDLE stdOutputHandle;

using namespace std;

// where is the builded exe
const string ExePath() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

// function for appending paths
string pathAppend(const string& p1, const string& p2) {

	char sep = '/';
	string tmp = p1;

#ifdef _WIN32
	sep = '\\';
#endif

	if (p1[p1.length()] != sep) { // Need to add a
		tmp += sep;                // path separator
		return(tmp + p2);
	}
	else
		return(p1 + p2);
}

// function that inverse the current playing color (black-to-white-to-black-to...)
void switchColorToPlay()
{
	(color_to_play == BLACK) ? color_to_play = WHITE : color_to_play = BLACK;
}

// function that make an initial order of figures
void initChessBoard()
{
	for (int x = 0; x < 8; x++)
	{
		(&chessboard_squares[x][7])->figure_type = first_row_figure_order[x];
		(&chessboard_squares[x][7])->figure_color = BLACK;

		(&chessboard_squares[x][6])->figure_type = PAWN;
		(&chessboard_squares[x][6])->figure_color = BLACK;

		(&chessboard_squares[x][1])->figure_type = PAWN;
		(&chessboard_squares[x][1])->figure_color = WHITE;

		(&chessboard_squares[x][0])->figure_type = first_row_figure_order[x];
		(&chessboard_squares[x][0])->figure_color = WHITE;
	}
}

// clear the screen and set the cursor to {0,0} position
void ClearScreen()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD count;
	DWORD cellCount;

	//stdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (stdOutputHandle == INVALID_HANDLE_VALUE)
		return;

	/* Get the number of cells in the current buffer */
	if (!GetConsoleScreenBufferInfo(stdOutputHandle, &csbi))
		return;

	cellCount = csbi.dwSize.X *csbi.dwSize.Y;

	/* Fill the entire buffer with spaces */
	if (!FillConsoleOutputCharacter(stdOutputHandle, (TCHAR) ' ', cellCount, homeCoords, &count))
		return;

	/* Fill the entire buffer with the current colors and attributes */
	if (!FillConsoleOutputAttribute(stdOutputHandle, csbi.wAttributes, cellCount, homeCoords, &count))
		return;

	/* Move the cursor home */
	SetConsoleCursorPosition(stdOutputHandle, homeCoords);
}

// set the output console window size, buffer size and font
void initConsole()
{
	stdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);  // Get Output Handle

	_COORD coordscreenBufferSize;
	coordscreenBufferSize.X = 18;
	coordscreenBufferSize.Y = 10;

	_SMALL_RECT rectConsoleWindowInfo;
	rectConsoleWindowInfo.Top = 0;
	rectConsoleWindowInfo.Left = 0;
	rectConsoleWindowInfo.Bottom = 10;
	rectConsoleWindowInfo.Right = 18;

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.cbSize = sizeof(cfi);
	cfi.dwFontSize.X = 32;
	cfi.dwFontSize.Y = 64;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"MS Gothic");  // set Font


	SetCurrentConsoleFontEx(stdOutputHandle, FALSE, &cfi);
	SetConsoleTextAttribute(stdOutputHandle, BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED); //FOREGROUND_RED | FOREGROUND_BLUE | 
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	SetConsoleScreenBufferSize(stdOutputHandle, coordscreenBufferSize);   // Set Window Buffer Size
	SetConsoleWindowInfo(stdOutputHandle, TRUE, &rectConsoleWindowInfo);  // Set Window Size
}

// print the "current user symbol" turn
void coutTurn()
{
	char color_char_to_print[FIGURE_CHAR_SIZE * 2 + 1];
	const char * status_to_print;
	memset(color_char_to_print, 0, FIGURE_CHAR_SIZE + 1);
	memcpy(color_char_to_print, white_n_black_figures + (rand() % 6 + (int)color_to_play * 6) * FIGURE_CHAR_SIZE, FIGURE_CHAR_SIZE);

	status_to_print = bottom_neutral_symbol;

	if (white_in_check)
		status_to_print = check_white_symbol;
	if (black_in_check)
		status_to_print = check_black_symbol;

	cout << status_to_print << columns_abcdefgh << endl << "                \r" << color_char_to_print << "'s turn: ";
}

// print the current situation on the board
void printChessBoard()
{
	FigureType figure;
	FigureColor color;

	ClearScreen();

	char char_to_print[FIGURE_CHAR_SIZE * 2 + 1];
	memset(char_to_print, 0, FIGURE_CHAR_SIZE + 1);

	for (int y = 8; y > 0; y--)
	{
		memcpy(char_to_print, rows_12345678 + (y - 1) * FIGURE_CHAR_SIZE, FIGURE_CHAR_SIZE);
		cout << char_to_print;

		for (int x = 0; x < 8; x++)
		{
			figure = (&chessboard_squares[x][y - 1])->figure_type;
			color = (&chessboard_squares[x][y - 1])->figure_color;

			if (figure == NO_FIGURE)
				memcpy(char_to_print, empty_white_n_black_chars + (int)(x % 2 == y % 2)* EMPTY_CHAR_SIZE, EMPTY_CHAR_SIZE);
			else
				memcpy(char_to_print, white_n_black_figures + ((int)figure + (int)color * 6) * FIGURE_CHAR_SIZE, FIGURE_CHAR_SIZE);

			cout << char_to_print;
		}

		cout << endl;
	}

	coutTurn();
}

// beep and pring the board
void beepNReloadTheScreen()
{
	cout << "\a";
	printChessBoard();
}

// replace "containers" of two squares
bool makeMoveByChessBoardSquares(ChessBoardSquare* from_square, ChessBoardSquare* to_square)
{
	to_square->figure_color = from_square->figure_color;
	to_square->figure_type = from_square->figure_type;

	from_square->figure_color = NO_COLOR;
	from_square->figure_type = NO_FIGURE;
	return true;
}

// try to move the king by specifying the coordinates
bool makeKingsMove(int from_x, int from_y, int to_x, int to_y)
{
	bool move_successfull = false;
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];

	if (
		(abs(to_x - from_x) == 1 && (abs(from_y - to_y) == 0))
		||
		(abs(to_x - from_x) == 0 && (abs(from_y - to_y) == 1))
		||
		(abs(to_x - from_x) == 1 && (abs(from_y - to_y) == 1))
		)
		move_successfull = makeMoveByChessBoardSquares(from_square, to_square);

	return move_successfull;
}

// try to move the queen by specifying the coordinates
bool makeQueensMove(int from_x, int from_y, int to_x, int to_y)
{
	bool invalid_move = false;
	bool move_successfull = false;
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];
	int x_increment;
	int y_increment;

	if (from_x != to_x || from_y != to_y)
	{

		if (from_x == to_x)
		{
			y_increment = (to_y - from_y) / (abs(to_y - from_y));
			for (int i = from_y + y_increment; i != to_y; i += y_increment)
				if (chessboard_squares[to_x][i].figure_color != NO_COLOR)
					invalid_move = true;
		}
		else
			if (from_y == to_y)
			{
				x_increment = (to_x - from_x) / (abs(to_x - from_x));
				for (int i = from_x + x_increment; i != to_x; i += x_increment)
					if (chessboard_squares[i][to_y].figure_color != NO_COLOR)
						invalid_move = true;
			}
			else
				if (abs(from_x - to_x) == abs(from_y - to_y))
				{
					x_increment = (to_x - from_x) / (abs(to_x - from_x));
					y_increment = (to_y - from_y) / (abs(to_y - from_y));

					for (int i = 1; i < abs(from_x - to_x); i++)
						if (chessboard_squares[from_x + x_increment * i][from_y + y_increment * i].figure_color != NO_COLOR)
							invalid_move = true;
				}
				else
					invalid_move = true;
	}
	else
		invalid_move = true;

	if (!invalid_move)
		move_successfull = makeMoveByChessBoardSquares(from_square, to_square);

	return move_successfull;
}

// try to move the bishop by specifying the coordinates
bool makeBishopsMove(int from_x, int from_y, int to_x, int to_y)
{
	bool invalid_move = false;
	bool move_successfull = false;
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];

	if (abs(from_x - to_x) == abs(from_y - to_y))
	{
		int x_increment = (to_x - from_x) / (abs(to_x - from_x));
		int y_increment = (to_y - from_y) / (abs(to_y - from_y));

		for (int i = 1; i < abs(from_x - to_x); i++)
			if (chessboard_squares[from_x + x_increment * i][from_y + y_increment * i].figure_color != NO_COLOR)
				invalid_move = true;
	}
	else
		invalid_move = true;

	if (!invalid_move)
		move_successfull = makeMoveByChessBoardSquares(from_square, to_square);

	return move_successfull;
}

// try to move the rook by specifying the coordinates
bool makeRooksMove(int from_x, int from_y, int to_x, int to_y)
{
	bool invalid_move = false;
	bool move_successfull = false;
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];
	int x_increment;
	int y_increment;

	if (from_x != to_x || from_y != to_y)
	{
		if (from_x == to_x)
		{
			y_increment = (to_y - from_y) / (abs(to_y - from_y));
			for (int i = from_y + y_increment; i != to_y; i += y_increment)
				if (chessboard_squares[to_x][i].figure_color != NO_COLOR)
					invalid_move = true;
		}
		else
			if (from_y == to_y)
			{
				x_increment = (to_x - from_x) / (abs(to_x - from_x));
				for (int i = from_x + x_increment; i != to_x; i += x_increment)
					if (chessboard_squares[i][to_y].figure_color != NO_COLOR)
						invalid_move = true;
			}
			else
				invalid_move = true;
	}
	else
		invalid_move = true;

	if (!invalid_move)
		move_successfull = makeMoveByChessBoardSquares(from_square, to_square);

	return move_successfull;
}

// try to move the knight by specifying the coordinates
bool makeKnightsMove(int from_x, int from_y, int to_x, int to_y)
{
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];
	bool move_successfull = false;

	if ((abs(from_x - to_x) == 2 && abs(from_y - to_y) == 1) || (abs(from_x - to_x) == 1 && abs(from_y - to_y) == 2))
		move_successfull = makeMoveByChessBoardSquares(from_square, to_square);

	return move_successfull;

}

// try to move the pawn by specifying the coordinates
bool makePawnsMove(int from_x, int from_y, int to_x, int to_y)
{
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];
	bool move_successfull = false;

	if ((((from_square->figure_color == WHITE)
		&& ((from_x == to_x && to_y == from_y + 2 && from_y == 1 && to_square->figure_color == NO_COLOR) // first row move
			|| (from_x == to_x && to_y == from_y + 1 && to_square->figure_color == NO_COLOR) // any row move
			|| ((from_x == to_x - 1 || from_x == to_x + 1) && from_y == to_y - 1 && to_square->figure_color == BLACK)))) // "eating" black figure
		|| (((from_square->figure_color == BLACK)
			&& ((from_x == to_x && to_y == from_y - 2 && from_y == 6 && to_square->figure_color == NO_COLOR) // first row move
				|| (from_x == to_x && to_y == from_y - 1 && to_square->figure_color == NO_COLOR) // any row move
				|| ((from_x == to_x - 1 || from_x == to_x + 1) && from_y == to_y + 1 && to_square->figure_color == WHITE))))) // "eating" white figure
		move_successfull = makeMoveByChessBoardSquares(from_square, to_square);

	return move_successfull;
}

// try to move the king by specifying the coordinates
bool makeMoveByCoordinates(int from_x, int from_y, int to_x, int to_y)
{
	ChessBoardSquare* from_square = &chessboard_squares[from_x][from_y];
	ChessBoardSquare* to_square = &chessboard_squares[to_x][to_y];

	// are we playing with a right color?
	if (from_square->figure_color != color_to_play)
		return false;

	// are we trying to move the figure to the square with same color?
	if (from_square->figure_color == to_square->figure_color && to_square->figure_color != NO_COLOR)
		return false;

	bool to_return = false;

	// find which figure will need to move
	switch (from_square->figure_type)
	{
	case KING:
		to_return = makeKingsMove(from_x, from_y, to_x, to_y);
		break;

	case QUEEN:
		to_return = makeQueensMove(from_x, from_y, to_x, to_y);
		break;

	case BISHOP:
		to_return = makeBishopsMove(from_x, from_y, to_x, to_y);
		break;

	case KNIGHT:
		to_return = makeKnightsMove(from_x, from_y, to_x, to_y);
		break;

	case ROOK:
		to_return = makeRooksMove(from_x, from_y, to_x, to_y);
		break;

	case PAWN:
		to_return = makePawnsMove(from_x, from_y, to_x, to_y);
		break;

	case NO_FIGURE:
		to_return = false;
		break;

	default:
		to_return = false;
		break;
	}

	return to_return;


}

// checking is the move possible by duplicating squares array, and return array in original state after checking
bool checkOnePossibleMove(int x1, int y1, int x2, int y2)
{
	ChessBoardSquare temp_chessboard_squares[8][8];
	bool possible_move = false;
	memcpy(temp_chessboard_squares, chessboard_squares, sizeof(chessboard_squares));
	possible_move = makeMoveByCoordinates(x1, y1, x2, y2);
	memcpy(chessboard_squares, temp_chessboard_squares, sizeof(chessboard_squares));
	return possible_move;
}

// function that save the game (chessboard array first, then specific booleans)
void saveTheGame(string strfilename)
{
	const char* filename = strfilename.c_str();
	cout << "saving";
	ofstream myfile(filename);

	for (int x = 0; x < 8; x++)
		for (int y = 0; y < 8; y++)
			myfile << chessboard_squares[x][y].figure_color << chessboard_squares[x][y].figure_type;

	myfile << color_to_play;
	myfile << whites_king_castling_figures_untouched;
	myfile << whites_queen_castling_figures_untouched;
	myfile << blacks_king_castling_figures_untouched;
	myfile << blacks_queen_castling_figures_untouched;
	myfile << white_in_check;
	myfile << black_in_check;

	myfile.close();
	beepNReloadTheScreen();
}

// function that load the game (chessboard array first, then specific booleans)
void loadTheGame(string strfilename)
{
	const char* filename = strfilename.c_str();
	cout << "loading";
	char a;
	FigureColor color;
	FigureType type;
	ifstream infile(filename);
	for (int x = 0; x < 8; x++)
		for (int y = 0; y < 8; y++)
		{
			infile >> a;
			switch (a)
			{
			case '0':
				color = WHITE;
				break;
			case '1':
				color = BLACK;
				break;
			case '2': default:
				color = NO_COLOR;
			}

			infile >> a;
			switch (a)
			{
			case '0':
				type = KING;
				break;
			case '1':
				type = QUEEN;
				break;
			case '2':
				type = BISHOP;
				break;
			case '3':
				type = KNIGHT;
				break;
			case '4':
				type = ROOK;
				break;
			case '5':
				type = PAWN;
				break;
			case '6': default:
				type = NO_FIGURE;
				break;
			}

			chessboard_squares[x][y].figure_color = color;
			chessboard_squares[x][y].figure_type = type;
		}


	infile >> a;
	(a == '1') ? color_to_play = BLACK : color_to_play = WHITE;

	infile >> a;
	(a == '1') ? whites_king_castling_figures_untouched = true : whites_king_castling_figures_untouched = false;

	infile >> a;
	(a == '1') ? whites_queen_castling_figures_untouched = true : whites_queen_castling_figures_untouched = false;

	infile >> a;
	(a == '1') ? blacks_king_castling_figures_untouched = true : blacks_king_castling_figures_untouched = false;

	infile >> a;
	(a == '1') ? blacks_queen_castling_figures_untouched = true : blacks_queen_castling_figures_untouched = false;

	infile >> a;
	(a == '1') ? white_in_check = true : white_in_check = false;

	infile >> a;
	(a == '1') ? black_in_check = true : black_in_check = false;

	beepNReloadTheScreen();
}

// append the strign to the log
void appendTheLog(string log_game_path, string key_sequence)
{
	ofstream myfile;
	myfile.open(log_game_path, ofstream::app);
	if (myfile.fail())
		ofstream myfile(log_game_path);
	myfile << key_sequence << "\n";
	myfile.close();
}

// count all possible next moves
int countAllPossibleMoves()
{
	int no_of_moves = 0;
	for (int x1 = 0; x1 < 8; x1++)
		for (int y1 = 0; y1 < 8; y1++)
			for (int x2 = 0; x2 < 8; x2++)
				for (int y2 = 0; y2 < 8; y2++)
					if (checkOnePossibleMove(x1, y1, x2, y2))
						no_of_moves++;

	return no_of_moves;
}

// check (and log) all possible next moves
void checkAllPossibleMoves(string log_game_path = "")
{
	string no_of_moves = to_string(countAllPossibleMoves());
	appendTheLog(log_game_path, "possible moves:"+ no_of_moves);
	for (int x1 = 0; x1 < 8; x1++)
		for (int y1 = 0; y1 < 8; y1++)
			for (int x2 = 0; x2 < 8; x2++)
				for (int y2 = 0; y2 < 8; y2++)
					if (checkOnePossibleMove(x1, y1, x2, y2))
					{
						string row0 = " ";
						char row1 = x1 + 65;
						char row2 = x2 + 65;
						appendTheLog(log_game_path, row0 + row1 + to_string(y1 + 1) + row2 + to_string(y2 + 1));
					}
}

// check is the color specified in check
void checkCheck(FigureColor color)
{
	white_in_check = false;
	black_in_check = false;
	int x = -1;
	int y = -1;

	// let's find where is the king
	for (int x1 = 0; x1 < 8; x1++)
		for (int y1 = 0; y1 < 8; y1++)
			if (((&chessboard_squares[x1][y1])->figure_type == KING) && ((&chessboard_squares[x1][y1])->figure_color == color))
			{
				x = x1;
				y = y1;
			}

	// switch the color
	switchColorToPlay();

	// try to go with other color to the king's position
	for (int x1 = 0; x1 < 8; x1++)
		for (int y1 = 0; y1 < 8; y1++)
			if (checkOnePossibleMove(x1, y1, x, y)) // if it is possible set the appropriate bool to true
				(color_to_play == BLACK) ? white_in_check = true : black_in_check = true;

	// switch the color back
	switchColorToPlay();
}

// print the info screen
void infoScreen()
{
	const char* bottom_neutral_symbol = "◯";
	const char* check_white_symbol = "◎";
	const char* check_black_symbol = "⦿";
	const char* no_winner = "☮";
	const char* white_wins = "☺☺";
	const char* black_wins = "☻☻";
	ClearScreen();

	cout << endl << bottom_neutral_symbol << "-free to play" << endl;
	cout << check_white_symbol << "-white in check" << endl;
	cout << check_black_symbol << "-black in check" << endl;
	cout << white_wins << "-white wins" << endl;
	cout << black_wins << "-black wins" << endl;
	cout << no_winner << "-no winner" << endl << endl;
	cout << "   press ESC" << endl << "  to continue";

	int get_key = 0;
	while (get_key != 27)
	{
		get_key = _getch();
		//cout << get_key;
	} 
	ClearScreen();
	beepNReloadTheScreen();
}

// print the intro screen
void introScreen()
{
	COORD randomCoords;
	do
	{
		SetConsoleCursorPosition(stdOutputHandle, homeCoords);
		cout << endl << "･･･Yet･･Another･･･" << endl << endl << "･･2･PLAYER･CHESS･･" << endl << endl << "･･F6-･quick･save･･" << endl << "･･F7-･quick･load･･" << endl << "･･･I-･info screen･" << endl << endl << "･･press･･any･key･･";
		Beep(rand() % 500 + 500, rand() % 750);
		randomCoords = { (rand() % 9) * 2, rand() % 10 };
		SetConsoleCursorPosition(stdOutputHandle, randomCoords);
		cout << "☺";
		randomCoords = { (rand() % 9) * 2, rand() % 10 };
		SetConsoleCursorPosition(stdOutputHandle, randomCoords);
		cout << "☻";
	} while (!_kbhit()); // wait for the "any key" is pressed
}

// function for playing one move
bool playOneMove(string save_game_path, string log_game_path)
{
	int from_x, to_x, from_y, to_y;
	bool current_move_finished = false;

	int get_key; // integer to store ascii code of the key pressed
	char key_pressed; // char to store char pressed
	string key_sequence; // string to store the current key sequence
	cout << "✓";  // fix blinking cursor

	printChessBoard();

	do
	{
		key_sequence = "";

		do
		{
			//  take the ascii of the pressed key
			get_key = _getch(); 

			//  for longer ascii key, read the second key
			if (get_key == 0 || get_key == 0xe0) 
			{
				get_key = _getch();

				// if F6 is pressed
				if (get_key == 64)
					saveTheGame(save_game_path);

				// if F7 is pressed
				if (get_key == 65)
					loadTheGame(save_game_path);
			}
			else
			{
				// take the upper char of the char pressed
				key_pressed = toupper(char(get_key)); 

				// if the pressed is sometwing between a-h, A-H, or 1-8
				if ((get_key < 58) && (get_key > 47) || (get_key < 73) && (get_key > 64) || (get_key < 105) && (get_key > 96))
				{
					cout << key_pressed;
					key_sequence += key_pressed;
				}

				// if the backspace is pressed
				if (get_key == 8)
				{
					beepNReloadTheScreen();
					key_sequence = "";
				}

				// "i" or "I"
				if (get_key == 73 || get_key == 105)
					infoScreen();
			}

		} while (get_key != 13); // ENTER

		if (key_sequence.length() == 4)
		{
			from_x = key_sequence[0] - (int)'A';
			to_x = key_sequence[2] - (int)'A';
			from_y = key_sequence[1] - (int)'1';
			to_y = key_sequence[3] - (int)'1';

			if (from_x < 0 || from_x > 7 || from_y < 0 || from_y > 7 || to_x < 0 || to_x > 7 || to_y < 0 || to_y > 8)
			{
				beepNReloadTheScreen();
				key_sequence = "";
			}
			else
			{
				// prepare to return the positin before the move
				ChessBoardSquare* from_current_square = &chessboard_squares[from_x][from_y];
				ChessBoardSquare* to_current_square = &chessboard_squares[to_x][to_y];
				FigureType from_type = from_current_square->figure_type;
				FigureColor from_color = from_current_square->figure_color;
				FigureType to_type = to_current_square->figure_type;
				FigureColor to_color = to_current_square->figure_color;

				if (makeMoveByCoordinates(from_x, from_y, to_x, to_y))
				{
					// if the move is possible
					// check is user still in chess
					bool still_in_check = false;
					if ((color_to_play == WHITE) && (white_in_check))
					{
						checkCheck(WHITE);
						if (white_in_check)
							still_in_check = true;
					}
					if ((color_to_play == BLACK) && (black_in_check))
					{
						checkCheck(BLACK);
						if (black_in_check)
							still_in_check = true;
					}
					if (!still_in_check)
					{
						// if the move is good
						appendTheLog(log_game_path, key_sequence);
						current_move_finished = true;
						switchColorToPlay();
					}
					else
					{
						// return to the position before the move
						cout << "\a";
						from_current_square->figure_type = from_type;
						from_current_square->figure_color = from_color;
						to_current_square->figure_type = to_type;
						to_current_square->figure_color = to_color;
						beepNReloadTheScreen();
						key_sequence = "";
					}
				}
				else
				{
					// if move is ot possible
					beepNReloadTheScreen();
					key_sequence = "";
				}
			}
		}

	} while (!current_move_finished);

	checkCheck(color_to_play);

	if (white_in_check || black_in_check)
		cout << "\a";

	checkAllPossibleMoves(log_game_path);
	int no_of_moves = countAllPossibleMoves();
	if (no_of_moves == 0)
		return false;

	return true;

}

// main loop function
void playChess()
{
	string current_dir;
	string save_game_dir;
	string save_game_path;
	string log_game_dir;
	string log_game_path;
	time_t rawtime;
	char buffer[80];
	tm timeinfo;

	int get_key;
	char newgame = 'Y';

	current_dir = ExePath();
	save_game_dir = pathAppend(current_dir, "SaveGames");
	save_game_path = pathAppend(save_game_dir, "last_game.chs");
	log_game_dir = pathAppend(current_dir, "Logs");

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S.log", &timeinfo);
	string dt_str(buffer);
	std::cout << dt_str;

	log_game_path = pathAppend(log_game_dir, dt_str);

	CreateDirectory(save_game_dir.c_str(), NULL);
	CreateDirectory(log_game_dir.c_str(), NULL);

	initConsole();
	system("cls");

	introScreen();

	system("cls");

	initChessBoard();



	do
	{
		while (playOneMove(save_game_path, log_game_path))
		{
			
		}

		cout << "New game?" << endl << "(y)es " <<endl <<"/ (n)o: ";
		get_key = _getch();

		if (char(toupper(get_key)) != 'Y')
			newgame = 'N';

	} while (newgame == 'Y');
}

// main function
int main()
{
	playChess();
	return 0;
}