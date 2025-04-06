# Data Type Testing
x = 10             # int
y = 3.14           # float
name = "Alice"     # string
is_valid = True    # boolean
items = [1, 2, 3]  # list

print("Testing Data Types:")
print(x)
print(y)
print(name)
print(is_valid)
print(items)

# Simple if
if x > 5:
    print("x is greater than 5")

# if-else
if y < 1:
    print("y is less than 1")
else:
    print("y is greater than or equal to 1")

# if-elif-else
if name == "Bob":
    print("Hello Bob")
elif name == "Alice":
    print("Hello Alice")
else:
    print("Hello Stranger")

# Nested if
if is_valid:
    if x == 10:
        print("Nested if working: x is 10 and valid")
    else:
        print("x is not 10 but valid")
else:
    print("Not valid")
