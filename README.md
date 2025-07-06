# HAVOC - Heap Allocated Value Object Containers

## What is HAVOC, and what is the use case?

Imagine the following scenario: You have a node tree, possibly an AST, which you want to express as a C++ object structure. If dealing with an AST, you want to be able to express it as closely as possible to your grammar. This probably means using either polymorphism or something like `std::variant` to express a choice between multiple nodes.

For example, if we're talking about a traditional C like language, you might have "binary expressions" and "unary expressions", both being "expressions" in your grammar. 

You could either represent them like this (the traditional approach)...

```cpp
struct expression
{
    virtual ~expression() = default;
};

struct binary_expression : expression
{
};

struct unary_expression : expression
{
};
```
...or like this:

```cpp
struct binary_expression
{
};

struct unary_expression
{
};

using expression = std::variant<binary_expression, unary_expression>;
```

I would argue that the later approach is preferable in a number of ways:

### You enforce non-nullability in your contract

If an expression is expected to be present according to the grammar, then this should be reflected in your C++ code as well. Using a polymorphic type would mean exposing it as an `std::unique_ptr<expression>`, for example, which can have a null value.

Using an `std::variant` here instead enforces that there can never be a null value.

### You can easily implement visitors without relying on RTTI, and without sacrificing flexibility

Using the polymorphic approach, you would either need to implement a visitor by using RTTI/`dynamic_cast`, or you would need to use vtables. Both approaches would end up performing the same as HAVOC, but would require a lot of boilerplate.

If you are familiar with the visitor pattern, you would likely go for the vtables solution. Here we would let `expression` have a method (commonly named `accept` in the visitor pattern), which would invoke the correct method of the visitor based on the concrete type of the object. Obviously, this approach would **not** let us vary the return type in C++, as the `accept` method cannot be both templated and virtual. We then end up with bad compromises such as having our visitor implementations store state, or returning `std::any` like ANTLR does.

Once again, using an `std::variant` here together with `std::visit` will work around this problem.

### Your contract better mirrors the grammar

I would argue that having something like...

```cpp
using expression = std::variant<binary_expression, unary_expression>;
```
...perfectly mirrors a grammar where an expression can be *either* a unary expression, or a binary expression.

And while object oriented programmers might be more used to working with inheritence, I think it would be hard to argue that it provides the same overview and 1:1 mapping to the grammar in this scenario.

