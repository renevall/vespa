// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchlib.expression;

import com.yahoo.vespa.objects.BufferSerializer;
import org.junit.Test;

import java.util.List;

import static org.hamcrest.core.Is.is;
import static org.hamcrest.core.IsNot.not;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

/**
 * @author Ulf Lilleengen
 * @since 5.1
 */
public class ResultNodeVectorTestCase extends ResultNodeTest {
    @Test
    public void testClassId() {
        assertThat(new IntegerResultNodeVector().getClassId(), is(IntegerResultNodeVector.classId));
        assertThat(new Int32ResultNodeVector().getClassId(), is(Int32ResultNodeVector.classId));
        assertThat(new Int16ResultNodeVector().getClassId(), is(Int16ResultNodeVector.classId));
        assertThat(new Int8ResultNodeVector().getClassId(), is(Int8ResultNodeVector.classId));
        assertThat(new FloatResultNodeVector().getClassId(), is(FloatResultNodeVector.classId));
        assertThat(new BoolResultNodeVector().getClassId(), is(BoolResultNodeVector.classId));
    }

    @Test
    public void testVectorAdd() {
        BoolResultNodeVector b = new BoolResultNodeVector();
        b.add(new BoolResultNode(true));
        b.add(new BoolResultNode(false));
        b.add((ResultNode)new BoolResultNode(false));
        assertThat(b.getVector().size(), is(3));

        Int8ResultNodeVector i8 = new Int8ResultNodeVector();
        i8.add(new Int8ResultNode((byte)9));
        i8.add(new Int8ResultNode((byte)2));
        i8.add((ResultNode)new Int8ResultNode((byte)5));
        assertThat(i8.getVector().size(), is(3));

        Int16ResultNodeVector i16 = new Int16ResultNodeVector();
        i16.add(new Int16ResultNode((short)9));
        i16.add(new Int16ResultNode((short)2));
        i16.add((ResultNode)new Int16ResultNode((short)5));
        assertThat(i16.getVector().size(), is(3));

        Int32ResultNodeVector i32 = new Int32ResultNodeVector();
        i32.add(new Int32ResultNode(9));
        i32.add(new Int32ResultNode(2));
        i32.add((ResultNode)new Int32ResultNode(5));
        assertThat(i32.getVector().size(), is(3));

        IntegerResultNodeVector ieger = new IntegerResultNodeVector();
        ieger.add(new IntegerResultNode(9));
        ieger.add(new IntegerResultNode(2));
        ieger.add((ResultNode)new IntegerResultNode(5));
        assertThat(ieger.getVector().size(), is(3));

        FloatResultNodeVector floatvec = new FloatResultNodeVector();
        floatvec.add(new FloatResultNode(3.3));
        floatvec.add(new FloatResultNode(3.4));
        floatvec.add((ResultNode)new FloatResultNode(3.5));
        assertThat(floatvec.getVector().size(), is(3));
    }

    @Test
    public void testCmp() {
        ResultNodeVector int8vec = new Int8ResultNodeVector().add(new Int8ResultNode((byte) 2));
        ResultNodeVector int8veclarge = new Int8ResultNodeVector().add(new Int8ResultNode((byte) 2)).add(new Int8ResultNode((byte) 5));
        ResultNodeVector int8vecsmall = new Int8ResultNodeVector().add(new Int8ResultNode((byte) 1));

        ResultNodeVector int16vec = new Int16ResultNodeVector().add(new Int16ResultNode((short) 2));
        ResultNodeVector int16veclarge = new Int16ResultNodeVector().add(new Int16ResultNode((short) 2)).add(new Int16ResultNode((short) 5));
        ResultNodeVector int16vecsmall = new Int16ResultNodeVector().add(new Int16ResultNode((short) 1));

        ResultNodeVector int32vec = new Int32ResultNodeVector().add(new Int32ResultNode(2));
        ResultNodeVector int32veclarge = new Int32ResultNodeVector().add(new Int32ResultNode(2)).add(new Int32ResultNode(5));
        ResultNodeVector int32vecsmall = new Int32ResultNodeVector().add(new Int32ResultNode(1));

        ResultNodeVector intvec = new IntegerResultNodeVector().add(new IntegerResultNode(2));
        ResultNodeVector intveclarge = new IntegerResultNodeVector().add(new IntegerResultNode(2)).add(new IntegerResultNode(5));
        ResultNodeVector intvecsmall = new IntegerResultNodeVector().add(new IntegerResultNode(1));

        FloatResultNodeVector floatvec = new FloatResultNodeVector().add(new FloatResultNode(2.2));
        FloatResultNodeVector floatveclarge = new FloatResultNodeVector().add(new FloatResultNode(2.2)).add(new FloatResultNode(5.5));
        FloatResultNodeVector floatvecsmall = new FloatResultNodeVector().add(new FloatResultNode(1.2));

        StringResultNodeVector strvec  = new StringResultNodeVector().add(new StringResultNode("foo"));
        StringResultNodeVector strveclarge  = new StringResultNodeVector().add(new StringResultNode("foolio"));
        StringResultNodeVector strvecsmall  = new StringResultNodeVector().add(new StringResultNode("bario"));

        RawResultNodeVector rawvec = new RawResultNodeVector().add(new RawResultNode(new byte[]{6, 9}));
        RawResultNodeVector rawveclarge = new RawResultNodeVector().add(new RawResultNode(new byte[]{9, 6}));
        RawResultNodeVector rawvecsmall = new RawResultNodeVector().add(new RawResultNode(new byte[]{6, 6}));

        assertClassCmp(int8vec);
        assertClassCmp(int16vec);
        assertClassCmp(int32vec);
        assertClassCmp(intvec);
        assertClassCmp(floatvec);
        assertClassCmp(strvec);
        assertClassCmp(rawvec);

        assertVecEqual(int8vec, int8vec);
        assertVecLt(int8vec, int8veclarge);
        assertVecGt(int8veclarge, int8vec);
        assertVecGt(int8vec, int8vecsmall);
        assertVecLt(int8vecsmall, int8vec);

        assertVecEqual(int16vec, int16vec);
        assertVecLt(int16vec, int16veclarge);
        assertVecGt(int16veclarge, int16vec);
        assertVecGt(int16vec, int16vecsmall);
        assertVecLt(int16vecsmall, int16vec);

        assertVecEqual(int32vec, int32vec);
        assertVecLt(int32vec, int32veclarge);
        assertVecGt(int32veclarge, int32vec);
        assertVecGt(int32vec, int32vecsmall);
        assertVecLt(int32vecsmall, int32vec);

        assertVecEqual(intvec, intvec);
        assertVecLt(intvec, intveclarge);
        assertVecGt(intveclarge, intvec);
        assertVecGt(intvec, intvecsmall);
        assertVecLt(intvecsmall, intvec);

        assertVecEqual(floatvec, floatvec);
        assertVecLt(floatvec, floatveclarge);
        assertVecGt(floatveclarge, floatvec);
        assertVecGt(floatvec, floatvecsmall);
        assertVecLt(floatvecsmall, floatvec);

        assertVecEqual(strvec, strvec);
        assertVecLt(strvec, strveclarge);
        assertVecGt(strveclarge, strvec);
        assertVecGt(strvec, strvecsmall);
        assertVecLt(strvecsmall, strvec);

        assertVecEqual(rawvec, rawvec);
        assertVecLt(rawvec, rawveclarge);
        assertVecGt(rawveclarge, rawvec);
        assertVecGt(rawvec, rawvecsmall);
        assertVecLt(rawvecsmall, rawvec);
    }

    private void assertVecLt(ResultNodeVector vec1, ResultNodeVector vec2) {
        assertTrue(vec1.onCmp(vec2) < 0);
    }

    private void assertVecGt(ResultNodeVector vec1, ResultNodeVector vec2) {
        assertTrue(vec1.onCmp(vec2) > 0);
    }

    private void assertVecEqual(ResultNodeVector vec1, ResultNodeVector vec2) {
        assertThat(vec1.onCmp(vec2), is(0));
    }

    private void assertClassCmp(ResultNodeVector add) {
        assertThat(add.onCmp(new NullResultNode()), is(not(0)));
    }

    @Test
    public void testSerialize() throws InstantiationException, IllegalAccessException {
        assertCorrectSerialization(new FloatResultNodeVector().add(new FloatResultNode(1.1)).add(new FloatResultNode(3.3)), new FloatResultNodeVector());
        assertCorrectSerialization(new IntegerResultNodeVector().add(new IntegerResultNode(1)).add(new IntegerResultNode(3)), new IntegerResultNodeVector());
        assertCorrectSerialization(new Int16ResultNodeVector().add(new Int16ResultNode((short) 1)).add(new Int16ResultNode((short) 3)), new Int16ResultNodeVector());
        assertCorrectSerialization(new Int8ResultNodeVector().add(new Int8ResultNode((byte) 1)).add(new Int8ResultNode((byte) 3)), new Int8ResultNodeVector());
        assertCorrectSerialization(new StringResultNodeVector().add(new StringResultNode("foo")).add(new StringResultNode("bar")), new StringResultNodeVector());
        assertCorrectSerialization(new RawResultNodeVector().add(new RawResultNode(new byte[]{6, 9})).add(new RawResultNode(new byte[]{9, 6})), new RawResultNodeVector());
        assertCorrectSerialization(new BoolResultNodeVector().add(new BoolResultNode(true)).add(new BoolResultNode(false)), new BoolResultNodeVector());
    }
}
