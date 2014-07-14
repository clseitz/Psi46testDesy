
// indent *.h
// indent *.cpp
// uses .indent.pro

// indentation style by example

int main ()
{
  // opening brace { for functions on separate line

  for( int i = 0; i < 10; ++i ) {
    cout << i << endl;
  }
  // for( like a function, blank after parenthesis (
  // lots of whitespace,
  // ++i is faster than i++,
  // opening brace { on same line as for
  // indentation is 2
  // closing brace } on separate line

  bool flag = true; // whitespace

  if( flag ) {
    cout << "true" << endl;
  }
  else {
    cout << "FALSe" << endl;
  }
  // if( like a function, blank after parenthesis (
  // opening brace { on same line as if
  // indentation is 2
  // closing brace } on separate line
  // else { on seprate line
}
