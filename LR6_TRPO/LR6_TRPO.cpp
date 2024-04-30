﻿#include <iostream>
#include <cassert>

struct Expression //базовая абстрактная структура "Выражение"
{
	virtual ~Expression() { } //виртуальный деструктор
	virtual double evaluate() const = 0; //абстрактный метод «вычислить»
};

struct Number : Expression // стуктура «Число»
{
	Number(double value) : value_(value) {} //конструктор
	double value() const { return value_; } // метод чтения значения числа
	double evaluate() const { return value_; } // реализация виртуального метода «вычислить»
	~Number() {}//деструктор, тоже виртуальный

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

private:
	std::string const name_; // имя функции 
	Expression const* arg_; // указатель на ее аргумент
};

struct Variable : Expression // структура «Переменная»
{
	Variable(std::string const& name) : name_(name) { } //в конструкторе надо указать ее имя
	std::string const& name() const { return name_; } // чтение имени переменной
	double evaluate() const { return 0.0; } // реализация виртуального метода «вычислить»
	
private:
	std::string const name_; // имя переменной
};

int main()
{
    std::cout << "Hello World!\n";
	Expression* e1 = new Number(1.234);
	Expression* e2 = new Number(-1.234);
	Expression* e3 = new BinaryOperation(e1, BinaryOperation::DIV, e2);
	std::cout << e3->evaluate() << std::endl;
	//------------------------------------------------------------------------------
	Expression* n32 = new Number(32.0);
	Expression* n16 = new Number(16.0);
	Expression* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
	Expression* callSqrt = new FunctionCall("sqrt", minus);
	Expression* n2 = new Number(2.0);
	Expression* mult = new BinaryOperation(n2, BinaryOperation::MUL, callSqrt);
	Expression* callAbs = new FunctionCall("abs", mult);
	std::cout << callAbs->evaluate() << std::endl;
	//------------------------------------------------------------------------------
	
}