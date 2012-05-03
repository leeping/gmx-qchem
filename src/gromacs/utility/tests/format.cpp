/*
 *
 *                This source code is part of
 *
 *                 G   R   O   M   A   C   S
 *
 *          GROningen MAchine for Chemical Simulations
 *
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2009, The GROMACS development team,
 * check out http://www.gromacs.org for more information.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 *
 * For more info, check our website at http://www.gromacs.org
 */
/*! \internal \file
 * \brief
 * Tests for string formatting functions and classes.
 *
 * For development, the tests can be run with a '-stdout' command-line option
 * to print out the help to stdout instead of using the XML reference
 * framework.
 *
 * \author Teemu Murtola <teemu.murtola@cbr.su.se>
 * \ingroup module_utility
 */
#include <string>
#include <vector>

#include <boost/scoped_ptr.hpp>
#include <gtest/gtest.h>

#include "gromacs/options/basicoptions.h"
#include "gromacs/options/options.h"
#include "gromacs/utility/format.h"

#include "testutils/refdata.h"
#include "testutils/testoptions.h"

namespace
{

using gmx::test::TestReferenceData;
using gmx::test::TestReferenceChecker;

class FormatTestBase : public ::testing::Test
{
    public:
        static void SetUpTestCase();

        static bool             s_bWriteToStdOut;

        TestReferenceChecker &checker();

        void checkFormatting(const std::string &text, const char *id);

    private:
        TestReferenceData       data_;
        boost::scoped_ptr<TestReferenceChecker> checker_;
};

bool FormatTestBase::s_bWriteToStdOut = false;

void FormatTestBase::SetUpTestCase()
{
    gmx::Options options(NULL, NULL);
    options.addOption(gmx::BooleanOption("stdout").store(&s_bWriteToStdOut));
    gmx::test::parseTestOptions(&options);
}

TestReferenceChecker &FormatTestBase::checker()
{
    if (checker_.get() == NULL)
    {
        checker_.reset(new TestReferenceChecker(data_.rootChecker()));
    }
    return *checker_;
}

void FormatTestBase::checkFormatting(const std::string &text, const char *id)
{
    if (s_bWriteToStdOut)
    {
        printf("%s:\n", id);
        printf("%s\n", text.c_str());
    }
    else
    {
        checker().checkStringBlock(text, id);
    }
}

/********************************************************************
 * Tests for formatString()
 */

TEST(FormatStringTest, HandlesBasicFormatting)
{
    EXPECT_EQ("12 abc", gmx::formatString("%d %s", 12, "abc"));
}

TEST(FormatStringTest, HandlesLongStrings)
{
    std::string longString = gmx::formatString("%*c%d", 2000, 'x', 10);
    EXPECT_EQ(2002U, longString.length());
    EXPECT_EQ("x10", longString.substr(1999));
}

/********************************************************************
 * Tests for concatenateStrings()
 */

typedef FormatTestBase ConcatenateStringsTest;

TEST_F(ConcatenateStringsTest, HandlesDifferentStringEndings)
{
    static const char * const strings[] = {
        "First string",
        "Second string ",
        "Third string\n",
        "Fourth string",
        ""
    };
    checkFormatting(gmx::concatenateStrings(strings), "CombinedStrings");
}

/********************************************************************
 * Tests for TextLineWrapper
 */

const char g_wrapText[] = "A quick brown fox jumps over the lazy dog";
const char g_wrapText2[] = "A quick brown fox jumps\nover the lazy dog";
const char g_wrapTextLongWord[]
    = "A quick brown fox jumps awordthatoverflowsaline over the lazy dog";

typedef FormatTestBase TextLineWrapperTest;

TEST_F(TextLineWrapperTest, HandlesEmptyStrings)
{
    gmx::TextLineWrapper wrapper;

    EXPECT_EQ("", wrapper.wrapToString(""));
    EXPECT_TRUE(wrapper.wrapToVector("").empty());
}

TEST_F(TextLineWrapperTest, HandlesTrailingNewlines)
{
    gmx::TextLineWrapper wrapper;

    EXPECT_EQ("line", wrapper.wrapToString("line"));
    EXPECT_EQ("line\n", wrapper.wrapToString("line\n"));
    EXPECT_EQ("line\n\n", wrapper.wrapToString("line\n\n"));
    EXPECT_EQ("\n", wrapper.wrapToString("\n"));
    EXPECT_EQ("\n\n", wrapper.wrapToString("\n\n"));
    {
        std::vector<std::string> wrapped(wrapper.wrapToVector("line"));
        ASSERT_EQ(1U, wrapped.size());
        EXPECT_EQ("line", wrapped[0]);
    }
    {
        std::vector<std::string> wrapped(wrapper.wrapToVector("line\n"));
        ASSERT_EQ(1U, wrapped.size());
        EXPECT_EQ("line", wrapped[0]);
    }
    {
        std::vector<std::string> wrapped(wrapper.wrapToVector("line\n\n"));
        ASSERT_EQ(2U, wrapped.size());
        EXPECT_EQ("line", wrapped[0]);
        EXPECT_EQ("", wrapped[1]);
    }
    {
        std::vector<std::string> wrapped(wrapper.wrapToVector("\n"));
        ASSERT_EQ(1U, wrapped.size());
        EXPECT_EQ("", wrapped[0]);
    }
    {
        std::vector<std::string> wrapped(wrapper.wrapToVector("\n\n"));
        ASSERT_EQ(2U, wrapped.size());
        EXPECT_EQ("", wrapped[0]);
        EXPECT_EQ("", wrapped[1]);
    }
}

TEST_F(TextLineWrapperTest, WrapsCorrectly)
{
    gmx::TextLineWrapper wrapper;

    wrapper.setLineLength(10);
    checkFormatting(wrapper.wrapToString(g_wrapText), "WrappedAt10");
    std::vector<std::string> wrapped(wrapper.wrapToVector(g_wrapText));
    checker().checkSequence(wrapped.begin(), wrapped.end(), "WrappedToVector");
    wrapper.setLineLength(13);
    checkFormatting(wrapper.wrapToString(g_wrapText), "WrappedAt13");
    wrapper.setLineLength(14);
    checkFormatting(wrapper.wrapToString(g_wrapText), "WrappedAt14");
    checkFormatting(wrapper.wrapToString(g_wrapTextLongWord), "WrappedWithLongWord");
}

TEST_F(TextLineWrapperTest, WrapsCorrectlyWithExistingBreaks)
{
    gmx::TextLineWrapper wrapper;

    checkFormatting(wrapper.wrapToString(g_wrapText2), "WrappedWithNoLimit");
    wrapper.setLineLength(10);
    checkFormatting(wrapper.wrapToString(g_wrapText2), "WrappedAt10");
    wrapper.setLineLength(14);
    checkFormatting(wrapper.wrapToString(g_wrapText2), "WrappedAt14");
}

/********************************************************************
 * Tests for TextTableFormatter
 */

class TextTableFormatterTest : public FormatTestBase
{
    public:
        TextTableFormatterTest();

        gmx::TextTableFormatter formatter_;
};

TextTableFormatterTest::TextTableFormatterTest()
{
    formatter_.addColumn("Col1", 4, false);
    formatter_.addColumn("Col2", 4, false);
    formatter_.addColumn("Col3Wrap", 14, true);
    formatter_.addColumn("Col4Wrap", 14, true);
}

TEST_F(TextTableFormatterTest, HandlesBasicCase)
{
    formatter_.clear();
    formatter_.addColumnLine(0, "foo");
    formatter_.addColumnLine(1, "bar");
    formatter_.addColumnLine(2, g_wrapText);
    formatter_.addColumnLine(3, g_wrapText2);
    checkFormatting(formatter_.formatRow(), "FormattedTable");
}

TEST_F(TextTableFormatterTest, HandlesOverflowingLines)
{
    formatter_.clear();
    formatter_.addColumnLine(0, "foobar");
    formatter_.addColumnLine(1, "barfoo");
    formatter_.addColumnLine(2, g_wrapText);
    formatter_.addColumnLine(3, g_wrapText2);
    checkFormatting(formatter_.formatRow(), "FormattedTable");
    formatter_.clear();
    formatter_.addColumnLine(0, "foobar");
    formatter_.addColumnLine(1, "barfoo");
    formatter_.setColumnFirstLineOffset(1, 1);
    formatter_.addColumnLine(2, g_wrapText);
    formatter_.addColumnLine(3, g_wrapText2);
    checkFormatting(formatter_.formatRow(), "FormattedRow2");
    formatter_.clear();
    formatter_.addColumnLine(0, "foobar");
    formatter_.addColumnLine(1, "barfoo");
    formatter_.setColumnFirstLineOffset(1, 1);
    formatter_.addColumnLine(2, g_wrapText);
    formatter_.setColumnFirstLineOffset(2, 2);
    formatter_.addColumnLine(3, g_wrapText2);
    checkFormatting(formatter_.formatRow(), "FormattedRow3");
}

TEST_F(TextTableFormatterTest, HandlesEmptyColumns)
{
    formatter_.clear();
    formatter_.addColumnLine(0, "foo");
    formatter_.addColumnLine(1, "bar");
    formatter_.addColumnLine(3, "Text");
    checkFormatting(formatter_.formatRow(), "FormattedTable");
    formatter_.clear();
    formatter_.addColumnLine(0, "foo");
    formatter_.addColumnLine(1, "bar");
    formatter_.setColumnFirstLineOffset(2, 1);
    formatter_.addColumnLine(3, "Text");
    checkFormatting(formatter_.formatRow(), "FormattedRow2");
    formatter_.clear();
    formatter_.addColumnLine(0, "foo");
    formatter_.addColumnLine(1, "bar");
    formatter_.addColumnLine(2, "");
    formatter_.setColumnFirstLineOffset(2, 1);
    formatter_.addColumnLine(3, "Text");
    checkFormatting(formatter_.formatRow(), "FormattedRow3");
}

} // namespace