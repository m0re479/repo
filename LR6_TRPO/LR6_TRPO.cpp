﻿#include <iostream>
#include <cassert>

struct Expression;
struct Number;
struct BinaryOperation;
struct FunctionCall;
struct Variable;

struct Transformer { //реализация паттерна проектирования Visitor
	virtual ~Transformer() {}
	virtual Expression* transformNumber(Number const*) = 0;
	virtual Expression* transformBinaryOperation(BinaryOperation const*) = 0;
	virtual Expression* transformFunctionCall(FunctionCall const*) = 0;
	virtual Expression* transformVariable(Variable const*) = 0;
};

struct Expression //базовая абстрактная структура "Выражение"
{
	virtual ~Expression() { } //виртуальный деструктор
	virtual double evaluate() const = 0; //абстрактный метод «вычислить»
	virtual Expression* transform(Transformer* tr) const = 0; // возвращает полностью новое АСД
	
};

struct Number : Expression // стуктура «Число»
{
	Number(double value) : value_(value) {} //конструктор
	double value() const { return value_; } // метод чтения значения числа
	double evaluate() const { return value_; } // реализация виртуального метода «вычислить»
	~Number() {}//деструктор, тоже виртуальный

	Expression* transform(Transformer* tr) const {
		return tr->transformNumber(this);
	}

private:
	double value_; // само вещественное число
};

struct BinaryOperation : Expression // «Бинарная операция»
{
	enum { // перечислим константы, которыми зашифруем символы операций 
		PLUS = '+',
		MINUS = '-',
		DIV = '/',
		MUL = '*'
	};
	// в конструкторе надо указать операнды и символ операции
	BinaryOperation(Expression const* left, int op, Expression const* right) : left_(left), op_(op), right_(right) {
		assert(left_ && right_);
	}
	~BinaryOperation() {
		delete left_;
		delete right_;
	}
	Expression const* left() const { return left_; } // чтение левого операнда
	Expression const* right() const { return right_; } // чтение правого операнда
	int operation() const { return op_; } // чтение символа операции
	double evaluate() const { // реализация виртуального метода «вычислить»
		double left = left_->evaluate(); // вычисляем левую часть
		double right = right_->evaluate(); // вычисляем правую часть
		switch (op_) {// в зависимости от вида операции выполняем вычисления
		case PLUS: return left + right;
		case MINUS: return left - right;
		case DIV: return left / right;
		case MUL: return left * right;
		}
	}

	Expression* transform(Transformer* tr) const {
		return tr->transformBinaryOperation(this);
	}

private:
	Expression const* left_; // указатель на левый операнд
	Expression const* right_; // указатель на правый операнд
	int op_; // символ операции
};

struct FunctionCall : Expression // структура «Вызов функции»
{
	// в конструкторе надо учесть имя функции и ее аогумент
	FunctionCall(std::string const& name, Expression const* arg) : name_(name), arg_(arg) {
		assert(arg_);
		assert(name_ == "sqrt" || name_ == "abs");
	} // разрешены только вызов sqrt и abs
	std::string const& name() const { return name_; }
	Expression const* arg() const { return arg_; } // чтение аргумента функции
	~FunctionCall() { delete arg_; }
	virtual double evaluate() const { // реализация виртуального метода «вычислить»
		if (name_ == "sqrt")
			return sqrt(arg_->evaluate()); // либо вычисляем корень квадратный
		else return fabs(arg_->evaluate()); // либо модуль — остальные функции запрещены
	}

	Expression* transform(Transformer* tr) const {
		return tr->transformFunctionCall(this);
	}


private:
	std::string const name_; // имя функции 
	Expression const* arg_; // указатель на ее аргумент
};

struct Variable : Expression // структура «Переменная»
{
	Variable(std::string const& name) : name_(name) { } //в конструкторе надо указать ее имя
	std::string const& name() const { return name_; } // чтение имени переменной
	double evaluate() const { return 0.0; } // реализация виртуального метода «вычислить»

	Expression* transform(Transformer* tr) const {
		return tr->transformVariable(this);
	}

private:
	std::string const name_; // имя переменной
};

struct CopySyntaxTree : Transformer {
	Expression* transformNumber(Number const* number) {
		return new Number(number->value());
	}

	Expression* transformBinaryOperation(BinaryOperation const* binop) {
		return new BinaryOperation(
			(binop->left())->transform(this),
			binop->operation(), 
			(binop->right())->transform(this)
		);
	}

	Expression* transformFunctionCall(FunctionCall const* fcall) {
		return new FunctionCall(
			fcall->name(),
			(fcall->arg())->transform(this)
		);
	}

	Expression* transformVariable(Variable const* var) {
		return new Variable(var->name());
	}
};

struct FoldConstants : Transformer {
	Expression* transformNumber(Number const* number) {//Просто число, преобразований не требуется
		return new Number(number->value());
	}
	Expression* transformBinaryOperation(BinaryOperation const* binop) {
		//Рассматриваем левое и правое выражения (с рекурсией)
		Expression* L = (binop->left())->transform(this);
		Expression* R = (binop->right())->transform(this);

		//Проверка, являются ли левое и правое выражения числами
		Number* L_numb = dynamic_cast<Number*>(L);
		Number* R_numb = dynamic_cast<Number*>(R);

		//Промежуточный узел дерева
		int Op = binop->operation();
		BinaryOperation* newBinop = new BinaryOperation(L, Op, R);

		if (L_numb && R_numb) {//оба явялются числами => нужно вычислить (свернуть)
			Expression* foldConst = new Number(newBinop->evaluate());
			delete newBinop; //узел больше не нужен
			return foldConst;
		}
		//Хотя бы одно из выражений не принадлежит типу Number
		return newBinop;

	}
	Expression* transformFunctionCall(FunctionCall const* fcall) {
		//Рассматриваем аргумент функции
		Expression* Arg = (fcall->arg())->transform(this);

		//Проверка, является ли аргумент числом
		Number* Arg_numb = dynamic_cast<Number*>(Arg);

		//Промежуточный узел дерева
		std::string Name = fcall->name();
		FunctionCall* newFcall = new FunctionCall(Name, Arg);

		if (Arg_numb) { //аргумент имеет тип Number => нужно вычислить (свернуть)
			Expression* foldConst = new Number(newFcall->evaluate());
			delete newFcall; //узел больше не нужен
			return foldConst;
		}
		return newFcall;
	}
	Expression* transformVariable(Variable const* var) { //Просто переменная, преобразований не требуется
		return new Variable(var->name());
	}
};

void printExpr(const Expression* expression) { //Вывод выражения на экран
	const Number* numb = dynamic_cast<const Number*>(expression);
	if (numb) {
		std::cout << numb->value();
	}
	else {
		const BinaryOperation* binop = dynamic_cast<const BinaryOperation*>(expression);
		if (binop) {
			printExpr(binop->left());
			std::cout << static_cast<char>(binop->operation());
			printExpr(binop->right());
		}
		else {
			const FunctionCall* funCall = dynamic_cast<const FunctionCall*>(expression);
			if (funCall) {
				std::cout << funCall->name() + "(";
				printExpr(funCall->arg());
				std::cout << ")";
			}
			else {
				const Variable* var = dynamic_cast<const Variable*>(expression);
				std::cout << var->name();
			}
		}
	}
}

int main()
{
	/*std::cout << "Hello World!\n";
	Expression* e1 = new Number(1.234);
	Expression* e2 = new Number(-1.234);
	Expression* e3 = new BinaryOperation(e1, BinaryOperation::DIV, e2);
	std::cout << e3->evaluate() << std::endl;*/
	//------------------------------------------------------------------------------
	/*Expression* n32 = new Number(32.0);
	Expression* n16 = new Number(16.0);
	Expression* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
	Expression* callSqrt = new FunctionCall("sqrt", minus);
	Expression* n2 = new Number(2.0);
	Expression* mult = new BinaryOperation(n2, BinaryOperation::MUL, callSqrt);
	Expression* callAbs = new FunctionCall("abs", mult);
	std::cout << callAbs->evaluate() << std::endl;*/
	//------------------------------------------------------------------------------
	/*Expression* expression = new Number(10.0);
	Transformer* transformer = new AconcreteTransformer();
	Expression* new_expression = expression->transform(transformer);*/
	//------------------------------------------------------------------------------
	//Проверка работы CopySyntaxTree
	/*Number* n32 = new Number(32.0);
	Number* n16 = new Number(16.0);
	BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
	std::cout << "minus = " << minus->evaluate() << std::endl;
	FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
	std::cout << "callSqrt = " << callSqrt->evaluate() << std::endl;
	Variable* var = new Variable("var");
	BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL, callSqrt);
	std::cout << "mult = " << mult->evaluate() << std::endl;
	FunctionCall* callAbs = new FunctionCall("abs", mult);
	std::cout << "callAbs = " << callAbs->evaluate() << std::endl;
	CopySyntaxTree CST;
	Expression* newExpr = callAbs->transform(&CST);
	std::cout << "newExpr = " << newExpr->evaluate() << std::endl;*/
	//------------------------------------------------------------------------------
	//Проверка работы FoldConstants
	Number* n32 = new Number(32.0);
	Number* n16 = new Number(16.0);
	BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
	FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
	Variable* var = new Variable("var");
	BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL, callSqrt);
	FunctionCall* callAbs = new FunctionCall("abs", mult);
	printExpr(callAbs);
	std::cout << std::endl;
	FoldConstants FC;
	Expression* newExpr = callAbs->transform(&FC);
	printExpr(newExpr);
}