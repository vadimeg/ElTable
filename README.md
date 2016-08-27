# ElTable - simplified electronic spreadsheet command line program.

Features:
Evaluates simple arithmetic expressions, resolves references, processes errors

Internals:

1. gets standard input (e.g. from text file)
2. fills out the table (cells) with raw values
3. runs evaluation process (calculating expressions and resolving
   references)
4. prints out the results

Executable with args example: eltab.exe < $(TargetDir)\test.elt
Example of the contents of the test.elt (cells are tab-delimited):

3	4
12 =C2	3	'Sample
=A1+B1*C1/5 =A2*B1 =B3-C3	'Spread
'Test	=4-3	5	'Sheet

First line means 3 raws and 4 columns.

Expected output:
12      -4      3       Sample
4       -16     -4      Spread
Test    1       5       Sheet

Note: if header points to more lines than available, the missing lines
are treated as empty cells.
