/*
 * testmatrix.cpp
 *
 * Copyright (C) 2006 by Universitaet Stuttgart (VIS). Alle Rechte vorbehalten.
 */

#include "testmatrix.h"

#include "testhelper.h"

#include <iostream>

#include "vislib/mathfunctions.h"
#include "vislib/Matrix.h"
#include "vislib/Matrix4.h"
#include "vislib/Polynom.h"
#include "vislib/Quaternion.h"


void TestMatrix(void) {
    using namespace std;
    using namespace vislib;
    using namespace vislib::math;
    
    Matrix<double, 4, COLUMN_MAJOR> m1;
    Matrix<double, 4, ROW_MAJOR> m2;
    Matrix<double, 4, COLUMN_MAJOR> m3;
    Matrix<double, 4, ROW_MAJOR> m4;
    Matrix4<double, COLUMN_MAJOR> m5;
    Matrix<double, 3, COLUMN_MAJOR> m6;

    ::AssertTrue("Default ctor creates id matrix.", m1.IsIdentity());
    ::AssertTrue("Default ctor creates id matrix.", m2.IsIdentity());
    ::AssertFalse("Default ctor creates no null matrix.", m1.IsNull());

    ::AssertTrue("Compare differently layouted matrices.", m1 == m2);

    ::AssertTrue("Compare Matrix<4> and Matrix4.", m1 == m5);

    ::AssertEqual("GetAt 0, 0", m1.GetAt(0, 0), 1.0);
    ::AssertEqual("Function call get at 0, 0", m1(0, 0), 1.0);
    ::AssertEqual("GetAt 1, 0", m1.GetAt(1, 0), 0.0);
    ::AssertEqual("Function call get at 1, 0", m1(1, 0), 0.0);

    m1.SetAt(0, 0, 0);
    ::AssertFalse("Id matrix destroyed.", m1.IsIdentity());

    ::AssertFalse("Compare differently layouted matrices.", m1 == m2);
    ::AssertTrue("Compare differently layouted matrices.", m1 != m2);

    m1.SetAt(0, 0, 1);
    ::AssertTrue("Id matrix restored.", m1.IsIdentity());

    m1.SetAt(0, 0, 0);
    ::AssertEqual("SetAt 0, 0", m1.GetAt(0, 0), 0.0);
    m1.SetAt(0, 1, 2.0);
    ::AssertEqual("SetAt 0, 1", m1.GetAt(0, 1), 2.0);
    m1.Transpose();
    ::AssertEqual("Transposed 0, 0", m1.GetAt(0, 0), 0.0);
    ::AssertNotEqual("Transposed 0, 1", m1.GetAt(0, 1), 2.0);
    ::AssertEqual("Transposed 1, 0", m1.GetAt(1, 0), 2.0);

    m1.SetIdentity();
    ::AssertTrue("Id matrix restored.", m1.IsIdentity());
    m1.Invert();
    ::AssertTrue("Inverting identity.", m1.IsIdentity());

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            m1.SetAt(r, c, static_cast<float>(c));
        }
    }
    m3 = m1;
    ::AssertFalse("Inverting uninvertable matrix.", m1.Invert());
    ::AssertEqual("Univertable matrix has determinant 0.", m1.Determinant(), 0.0);
    ::AssertEqual("Matrix unchanged after invert failed.", m1, m3);

    m1.SetAt(0, 0, 1.0);
    m1.SetAt(1, 0, 2.0);
    m1.SetAt(2, 0, 3.0);
    m1.SetAt(3, 0, 4.0);

    m1.SetAt(0, 1, 4.0);
    m1.SetAt(1, 1, 1.0);
    m1.SetAt(2, 1, 2.0);
    m1.SetAt(3, 1, 3.0);

    m1.SetAt(0, 2, 3.0);
    m1.SetAt(1, 2, 4.0);
    m1.SetAt(2, 2, 1.0);
    m1.SetAt(3, 2, 2.0);

    m1.SetAt(0, 3, 2.0);
    m1.SetAt(1, 3, 3.0);
    m1.SetAt(2, 3, 4.0);
    m1.SetAt(3, 3, 1.0);

    m1.Dump(cout);

    m3 = m1;
    ::AssertEqual("Assignment.", m1, m3);

    ::AssertNearlyEqual("Determinant", m3.Determinant(), -160.0);

    m2 = m1;
    ::AssertTrue("Converstion assignment.", m1 == m2);

    m4 = m2;
    ::AssertEqual("Assignment.", m2, m4);

    ::AssertEqual("m1 @ 0, 0", m1.GetAt(0, 0), 1.0);
    ::AssertEqual("m1 @ 1, 0", m1.GetAt(1, 0), 2.0);
    ::AssertEqual("m1 @ 2, 0", m1.GetAt(2, 0), 3.0);
    ::AssertEqual("m1 @ 3, 0", m1.GetAt(3, 0), 4.0);

    ::AssertEqual("m1 @ 0, 1", m1.GetAt(0, 1), 4.0);
    ::AssertEqual("m1 @ 1, 1", m1.GetAt(1, 1), 1.0);
    ::AssertEqual("m1 @ 2, 1", m1.GetAt(2, 1), 2.0);
    ::AssertEqual("m1 @ 3, 1", m1.GetAt(3, 1), 3.0);

    ::AssertEqual("m1 @ 0, 2", m1.GetAt(0, 2), 3.0);
    ::AssertEqual("m1 @ 1, 2", m1.GetAt(1, 2), 4.0);
    ::AssertEqual("m1 @ 2, 2", m1.GetAt(2, 2), 1.0);
    ::AssertEqual("m1 @ 3, 2", m1.GetAt(3, 2), 2.0);

    ::AssertEqual("m1 @ 0, 3", m1.GetAt(0, 3), 2.0);
    ::AssertEqual("m1 @ 1, 3", m1.GetAt(1, 3), 3.0);
    ::AssertEqual("m1 @ 2, 3", m1.GetAt(2, 3), 4.0);
    ::AssertEqual("m1 @ 3, 3", m1.GetAt(3, 3), 1.0);

    ::AssertEqual("m1 @ 0, 0 == @ 0", m1.GetAt(0, 0), m1.PeekComponents()[0 + 0 * 4]);
    ::AssertEqual("m1 @ 1, 0 == @ 1", m1.GetAt(1, 0), m1.PeekComponents()[1 + 0 * 4]);
    ::AssertEqual("m1 @ 2, 0 == @ 2", m1.GetAt(2, 0), m1.PeekComponents()[2 + 0 * 4]);
    ::AssertEqual("m1 @ 3, 0 == @ 3", m1.GetAt(3, 0), m1.PeekComponents()[3 + 0 * 4]);

    ::AssertEqual("m1 @ 0, 1 == @ 4", m1.GetAt(0, 1), m1.PeekComponents()[0 + 1 * 4]);
    ::AssertEqual("m1 @ 1, 1 == @ 5", m1.GetAt(1, 1), m1.PeekComponents()[1 + 1 * 4]);
    ::AssertEqual("m1 @ 2, 1 == @ 6", m1.GetAt(2, 1), m1.PeekComponents()[2 + 1 * 4]);
    ::AssertEqual("m1 @ 3, 1 == @ 7", m1.GetAt(3, 1), m1.PeekComponents()[3 + 1 * 4]);

    ::AssertEqual("m1 @ 0, 2 == @ 8", m1.GetAt(0, 2), m1.PeekComponents()[0 + 2 * 4]);
    ::AssertEqual("m1 @ 1, 2 == @ 9", m1.GetAt(1, 2), m1.PeekComponents()[1 + 2 * 4]);
    ::AssertEqual("m1 @ 2, 2 == @ 10", m1.GetAt(2, 2), m1.PeekComponents()[2 + 2 * 4]);
    ::AssertEqual("m1 @ 3, 2 == @ 11", m1.GetAt(3, 2), m1.PeekComponents()[3 + 2 * 4]);

    ::AssertEqual("m1 @ 0, 3 == @ 12", m1.GetAt(0, 3), m1.PeekComponents()[0 + 3 * 4]);
    ::AssertEqual("m1 @ 1, 3 == @ 13", m1.GetAt(1, 3), m1.PeekComponents()[1 + 3 * 4]);
    ::AssertEqual("m1 @ 2, 3 == @ 14", m1.GetAt(2, 3), m1.PeekComponents()[2 + 3 * 4]);
    ::AssertEqual("m1 @ 3, 3 == @ 15", m1.GetAt(3, 3), m1.PeekComponents()[3 + 3 * 4]);


    ::AssertEqual("m1 @ 0, 0 == @ 0", m2.GetAt(0, 0), m2.PeekComponents()[0 * 4 + 0]);
    ::AssertEqual("m1 @ 1, 0 == @ 4", m2.GetAt(1, 0), m2.PeekComponents()[1 * 4 + 0]);
    ::AssertEqual("m1 @ 2, 0 == @ 8", m2.GetAt(2, 0), m2.PeekComponents()[2 * 4 + 0]);
    ::AssertEqual("m1 @ 3, 0 == @ 12", m2.GetAt(3, 0), m2.PeekComponents()[3 * 4 + 0]);

    ::AssertEqual("m1 @ 0, 1 == @ 1", m2.GetAt(0, 1), m2.PeekComponents()[0 * 4 + 1]);
    ::AssertEqual("m1 @ 1, 1 == @ 5", m2.GetAt(1, 1), m2.PeekComponents()[1 * 4 + 1]);
    ::AssertEqual("m1 @ 2, 1 == @ 9", m2.GetAt(2, 1), m2.PeekComponents()[2 * 4 + 1]);
    ::AssertEqual("m1 @ 3, 1 == @ 13", m2.GetAt(3, 1), m2.PeekComponents()[3 * 4 + 1]);

    ::AssertEqual("m1 @ 0, 2 == @ 2", m2.GetAt(0, 2), m2.PeekComponents()[0 * 4 + 2]);
    ::AssertEqual("m1 @ 1, 2 == @ 6", m2.GetAt(1, 2), m2.PeekComponents()[1 * 4 + 2]);
    ::AssertEqual("m1 @ 2, 2 == @ 10", m2.GetAt(2, 2), m2.PeekComponents()[2 * 4 + 2]);
    ::AssertEqual("m1 @ 3, 2 == @ 14", m2.GetAt(3, 2), m2.PeekComponents()[3 * 4 + 2]);

    ::AssertEqual("m1 @ 0, 3 == @ 3", m2.GetAt(0, 3), m2.PeekComponents()[0 * 4 + 3]);
    ::AssertEqual("m1 @ 1, 3 == @ 7", m2.GetAt(1, 3), m2.PeekComponents()[1 * 4 + 3]);
    ::AssertEqual("m1 @ 2, 3 == @ 11", m2.GetAt(2, 3), m2.PeekComponents()[2 * 4 + 3]);
    ::AssertEqual("m1 @ 3, 3 == @ 15", m2.GetAt(3, 3), m2.PeekComponents()[3 * 4 + 3]);


    ::AssertTrue("Invert matrix.", m1.Invert());
    ::AssertNearlyEqual("Inverted @ 0, 0.", m1.GetAt(0, 0), -9.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 0.", m1.GetAt(1, 0), 11.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 0.", m1.GetAt(2, 0), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 0.", m1.GetAt(3, 0), 1.0 / 40.0);

    ::AssertNearlyEqual("Inverted @ 0, 1.", m1.GetAt(0, 1), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 1.", m1.GetAt(1, 1), -9.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 1.", m1.GetAt(2, 1), 11.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 1.", m1.GetAt(3, 1), 1.0 / 40.0);

    ::AssertNearlyEqual("Inverted @ 0, 2.", m1.GetAt(0, 2), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 2.", m1.GetAt(1, 2), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 2.", m1.GetAt(2, 2), -9.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 2.", m1.GetAt(3, 2), 11.0 / 40.0);

    ::AssertNearlyEqual("Inverted @ 0, 3.", m1.GetAt(0, 3), 11.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 3.", m1.GetAt(1, 3), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 3.", m1.GetAt(2, 3), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 3.", m1.GetAt(3, 3), -9.0 / 40.0);

    ::AssertTrue("Invert matrix.", m2.Invert());
    ::AssertNearlyEqual("Inverted @ 0, 0.", m2.GetAt(0, 0), -9.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 0.", m2.GetAt(1, 0), 11.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 0.", m2.GetAt(2, 0), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 0.", m2.GetAt(3, 0), 1.0 / 40.0);

    ::AssertNearlyEqual("Inverted @ 0, 1.", m2.GetAt(0, 1), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 1.", m2.GetAt(1, 1), -9.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 1.", m2.GetAt(2, 1), 11.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 1.", m2.GetAt(3, 1), 1.0 / 40.0);

    ::AssertNearlyEqual("Inverted @ 0, 2.", m2.GetAt(0, 2), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 2.", m2.GetAt(1, 2), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 2.", m2.GetAt(2, 2), -9.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 2.", m2.GetAt(3, 2), 11.0 / 40.0);

    ::AssertNearlyEqual("Inverted @ 0, 3.", m2.GetAt(0, 3), 11.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 1, 3.", m2.GetAt(1, 3), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 2, 3.", m2.GetAt(2, 3), 1.0 / 40.0);
    ::AssertNearlyEqual("Inverted @ 3, 3.", m2.GetAt(3, 3), -9.0 / 40.0);

    m3 *= m1;
    ::AssertTrue("m * m^-1 = id", m3.IsIdentity());
    
    m4 = m4 * m2;
    ::AssertTrue("m * m^-1 = id", m4.IsIdentity());


    Vector<double, 3> axis(1.0, 0.0, 0.0);
    Quaternion<double> q1(1.0, axis);
    Matrix4<double, COLUMN_MAJOR> rm1(q1);

    ::AssertNearlyEqual("Rotation from quaterion @ 0, 0.", rm1.GetAt(0, 0), 1.0);
    ::AssertNearlyEqual("Rotation from quaterion @ 1, 0.", rm1.GetAt(1, 0), 0.0);
    ::AssertNearlyEqual("Rotation from quaterion @ 2, 0.", rm1.GetAt(2, 0), 0.0);
    ::AssertNearlyEqual("Rotation from quaterion @ 3, 0.", rm1.GetAt(3, 0), 0.0);

    ::AssertNearlyEqual("Rotation from quaterion @ 0, 1.", rm1.GetAt(0, 1), 0.0);
    ::AssertNearlyEqual<double>("Rotation from quaterion @ 1, 1.", rm1.GetAt(1, 1), cos(1.0));
    ::AssertNearlyEqual<double>("Rotation from quaterion @ 2, 1.", rm1.GetAt(2, 1), sin(1.0));
    ::AssertNearlyEqual("Rotation from quaterion @ 3, 1.", rm1.GetAt(3, 1), 0.0);

    ::AssertNearlyEqual("Rotation from quaterion @ 0, 2.", rm1.GetAt(0, 2), 0.0);
    ::AssertNearlyEqual<double>("Rotation from quaterion @ 1, 2.", rm1.GetAt(1, 2), -sin(1.0));
    ::AssertNearlyEqual<double>("Rotation from quaterion @ 2, 2.", rm1.GetAt(2, 2), cos(1.0));
    ::AssertNearlyEqual("Rotation from quaterion @ 3, 2.", rm1.GetAt(3, 2), 0.0);

    ::AssertNearlyEqual("Rotation from quaterion @ 0, 3.", rm1.GetAt(0, 3), 0.0);
    ::AssertNearlyEqual("Rotation from quaterion @ 1, 3.", rm1.GetAt(1, 3), 0.0);
    ::AssertNearlyEqual("Rotation from quaterion @ 2, 3.", rm1.GetAt(2, 3), 0.0);
    ::AssertNearlyEqual("Rotation from quaterion @ 3, 3.", rm1.GetAt(3, 3), 1.0);

/*
http://de.wikipedia.org/wiki/Charakteristisches_Polynom
0 2 -1
2 -1 1
2 -1 3
charakteristisches Polynom:
    -x^3 + 2x^2 + 4x - 8
reelle Eigenwerte:
    {-2, 2, (2)}
Eigenvektor zu Eigenwert -2:
    {-3, 4, 2}
Eigenvektor zu Eigenwert 2:
    {1, 0, -2}
*/
    m6.SetAt(0, 0, 0.0);
    m6.SetAt(1, 0, 2.0);
    m6.SetAt(2, 0, -1.0);

    m6.SetAt(0, 1, 2.0);
    m6.SetAt(1, 1, -1.0);
    m6.SetAt(2, 1, 1.0);

    m6.SetAt(0, 2, 2.0);
    m6.SetAt(1, 2, -1.0);
    m6.SetAt(2, 2, 3.0);

    Polynom<double, 3> cp6(m6.CharacteristicPolynom());
    if (IsEqual(cp6[3], 1.0)) cp6 *= -1.0;
    AssertNearlyEqual("Coefficient a0 = -8", cp6[0], -8.0);
    AssertNearlyEqual("Coefficient a1 = 4", cp6[1], 4.0);
    AssertNearlyEqual("Coefficient a2 = 2", cp6[2], 2.0);
    AssertNearlyEqual("Coefficient a3 = -1", cp6[3], -1.0);

    double ev[4];
    unsigned int evc;
    double evt;

    evc = cp6.FindRoots(ev, 4);
    AssertEqual("Found two eigenvalues", evc, 2U);

    evt = 2.0;
    AssertTrue("Eigenvalue 2 found", IsEqual(ev[0], evt) || IsEqual(ev[1], evt));
    evt = -2.0;
    AssertTrue("Eigenvalue -2 found", IsEqual(ev[0], evt) || IsEqual(ev[1], evt));


/*
http://www.arndt-bruenner.de/mathe/scripts/eigenwert.htm
  -3  -4  -8   4
   5  -7   8   1
  -5   4  -7   7
  -3   5   7  -6
charakteristisches Polynom:
    x^4 + 23x^3 + 99x^2 - 675x - 3416
reelle Eigenwerte:
    {-12,589159228961312; -11,096724174256692; -4,613900261118899; 5,299783664336905}
Eigenvektor zu Eigenwert -12,589159228961312: 
    (0,05939964936610216; -0,8369818269339811; 0,5360406456563438; 0,0927012903997002)
Eigenvektor zu Eigenwert -11,096724174256692: 
    (-0,06271378020655696; -0,8667725283798831; 0,4604325354943767; 0,18103658767323305)
Eigenvektor zu Eigenwert -4,613900261118899: 
    (0,6759032660943609; 0,7055384845142796; -0,21288595069057908; 0,007056468722919981)
Eigenvektor zu Eigenwert 5,299783664336905: 
    (-0,4245882668759824; 0,2734502816046086; 0,6093868049835593; 0,611226201200128)
*/
    m1.SetAt(0, 0, -3.0);
    m1.SetAt(1, 0, -4.0);
    m1.SetAt(2, 0, -8.0);
    m1.SetAt(3, 0, 4.0);

    m1.SetAt(0, 1, 5.0);
    m1.SetAt(1, 1, -7.0);
    m1.SetAt(2, 1, 8.0);
    m1.SetAt(3, 1, 1.0);

    m1.SetAt(0, 2, -5.0);
    m1.SetAt(1, 2, 4.0);
    m1.SetAt(2, 2, -7.0);
    m1.SetAt(3, 2, 7.0);

    m1.SetAt(0, 3, -3.0);
    m1.SetAt(1, 3, 5.0);
    m1.SetAt(2, 3, 7.0);
    m1.SetAt(3, 3, -6.0);

    Polynom<double, 4> cp1(m1.CharacteristicPolynom());
    if (IsEqual(cp1[4], -1.0)) cp1 *= -1.0;
    AssertNearlyEqual("Coefficient a0 = -3416", cp1[0], -3416.0);
    AssertNearlyEqual("Coefficient a1 = -675", cp1[1], -675.0);
    AssertNearlyEqual("Coefficient a2 = 99", cp1[2], 99.0);
    AssertNearlyEqual("Coefficient a3 = 23", cp1[3], 23.0);
    AssertNearlyEqual("Coefficient a4 = 1", cp1[4], 1.0);

    evc = cp1.FindRoots(ev, 4);
    AssertEqual("Found four eigenvalues", evc, 4U);

    evt = -12.589159228961312;
    AssertTrue("Eigenvalue -12.589159228961312 found", IsEqual(ev[0], evt)
        || IsEqual(ev[1], evt) || IsEqual(ev[2], evt)
        || IsEqual(ev[3], evt));
    evt = -11.096724174256692;
    AssertTrue("Eigenvalue -11.096724174256692 found", IsEqual(ev[0], evt)
        || IsEqual(ev[1], evt) || IsEqual(ev[2], evt)
        || IsEqual(ev[3], evt));
    evt = -4.613900261118899;
    AssertTrue("Eigenvalue -4.613900261118899 found", IsEqual(ev[0], evt)
        || IsEqual(ev[1], evt) || IsEqual(ev[2], evt)
        || IsEqual(ev[3], evt));
    evt = 5.299783664336905;
    AssertTrue("Eigenvalue 5.299783664336905 found", IsEqual(ev[0], evt)
        || IsEqual(ev[1], evt) || IsEqual(ev[2], evt)
        || IsEqual(ev[3], evt));

}
