﻿/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/transient/file_utils.h"

#include <string.h>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/system_wrappers/include/file_wrapper.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#include "webrtc/typedefs.h"

namespace webrtc {

static const uint8_t kPiBytesf[4] = {0xDB, 0x0F, 0x49, 0x40};
static const uint8_t kEBytesf[4] = {0x54, 0xF8, 0x2D, 0x40};
static const uint8_t kAvogadroBytesf[4] = {0x2F, 0x0C, 0xFF, 0x66};

static const uint8_t kPiBytes[8] =
    {0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40};
static const uint8_t kEBytes[8] =
    {0x69, 0x57, 0x14, 0x8B, 0x0A, 0xBF, 0x05, 0x40};
static const uint8_t kAvogadroBytes[8] =
    {0xF4, 0xBC, 0xA8, 0xDF, 0x85, 0xE1, 0xDF, 0x44};

static const double kPi = 3.14159265358979323846;
static const double kE = 2.71828182845904523536;
static const double kAvogadro = 602214100000000000000000.0;

class TransientFileUtilsTest: public ::testing::Test {
 protected:
  TransientFileUtilsTest()
      : kTestFileName(
            test::ResourcePath("audio_processing/transient/double-utils",
                               "dat")),
        kTestFileNamef(
            test::ResourcePath("audio_processing/transient/float-utils",
                               "dat")) {}
  // This file (used in some tests) contains binary data. The data correspond to
  // the double representation of the constants: Pi, E, and the Avogadro's
  // Number;
  // appended in that order.
  const std::string kTestFileName;

  // This file (used in some tests) contains binary data. The data correspond to
  // the float representation of the constants: Pi, E, and the Avogadro's
  // Number;
  // appended in that order.
  const std::string kTestFileNamef;
};

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ConvertByteArrayToFloat)) {
  float value = 0.0;

  EXPECT_EQ(0, ConvertByteArrayToFloat(kPiBytesf, &value));
  EXPECT_FLOAT_EQ(kPi, value);

  EXPECT_EQ(0, ConvertByteArrayToFloat(kEBytesf, &value));
  EXPECT_FLOAT_EQ(kE, value);

  EXPECT_EQ(0, ConvertByteArrayToFloat(kAvogadroBytesf, &value));
  EXPECT_FLOAT_EQ(kAvogadro, value);
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ConvertByteArrayToDouble)) {
  double value = 0.0;

  EXPECT_EQ(0, ConvertByteArrayToDouble(kPiBytes, &value));
  EXPECT_DOUBLE_EQ(kPi, value);

  EXPECT_EQ(0, ConvertByteArrayToDouble(kEBytes, &value));
  EXPECT_DOUBLE_EQ(kE, value);

  EXPECT_EQ(0, ConvertByteArrayToDouble(kAvogadroBytes, &value));
  EXPECT_DOUBLE_EQ(kAvogadro, value);
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ConvertFloatToByteArray)) {
  rtc::scoped_ptr<uint8_t[]> bytes(new uint8_t[4]);

  EXPECT_EQ(0, ConvertFloatToByteArray(kPi, bytes.get()));
  EXPECT_EQ(0, memcmp(bytes.get(), kPiBytesf, 4));

  EXPECT_EQ(0, ConvertFloatToByteArray(kE, bytes.get()));
  EXPECT_EQ(0, memcmp(bytes.get(), kEBytesf, 4));

  EXPECT_EQ(0, ConvertFloatToByteArray(kAvogadro, bytes.get()));
  EXPECT_EQ(0, memcmp(bytes.get(), kAvogadroBytesf, 4));
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ConvertDoubleToByteArray)) {
  rtc::scoped_ptr<uint8_t[]> bytes(new uint8_t[8]);

  EXPECT_EQ(0, ConvertDoubleToByteArray(kPi, bytes.get()));
  EXPECT_EQ(0, memcmp(bytes.get(), kPiBytes, 8));

  EXPECT_EQ(0, ConvertDoubleToByteArray(kE, bytes.get()));
  EXPECT_EQ(0, memcmp(bytes.get(), kEBytes, 8));

  EXPECT_EQ(0, ConvertDoubleToByteArray(kAvogadro, bytes.get()));
  EXPECT_EQ(0, memcmp(bytes.get(), kAvogadroBytes, 8));
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ReadInt16BufferFromFile)) {
  std::string test_filename = kTestFileName;

  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  file->OpenFile(test_filename.c_str(),
                 true,    // Read only.
                 true,    // Loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kTestFileName.c_str();

  const size_t kBufferLength = 12;
  rtc::scoped_ptr<int16_t[]> buffer(new int16_t[kBufferLength]);

  EXPECT_EQ(kBufferLength, ReadInt16BufferFromFile(file.get(),
                                                   kBufferLength,
                                                   buffer.get()));
  EXPECT_EQ(22377, buffer[4]);
  EXPECT_EQ(16389, buffer[7]);
  EXPECT_EQ(17631, buffer[kBufferLength - 1]);

  file->Rewind();

  // The next test is for checking the case where there are not as much data as
  // needed in the file, but reads to the end, and it returns the number of
  // int16s read.
  const size_t kBufferLenghtLargerThanFile = kBufferLength * 2;
  buffer.reset(new int16_t[kBufferLenghtLargerThanFile]);
  EXPECT_EQ(kBufferLength, ReadInt16BufferFromFile(file.get(),
                                                   kBufferLenghtLargerThanFile,
                                                   buffer.get()));
  EXPECT_EQ(11544, buffer[0]);
  EXPECT_EQ(22377, buffer[4]);
  EXPECT_EQ(16389, buffer[7]);
  EXPECT_EQ(17631, buffer[kBufferLength - 1]);
}

TEST_F(TransientFileUtilsTest,
       DISABLED_ON_IOS(ReadInt16FromFileToFloatBuffer)) {
  std::string test_filename = kTestFileName;

  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  file->OpenFile(test_filename.c_str(),
                 true,    // Read only.
                 true,    // Loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kTestFileName.c_str();

  const size_t kBufferLength = 12;
  rtc::scoped_ptr<float[]> buffer(new float[kBufferLength]);

  EXPECT_EQ(kBufferLength, ReadInt16FromFileToFloatBuffer(file.get(),
                                                          kBufferLength,
                                                          buffer.get()));

  EXPECT_DOUBLE_EQ(11544, buffer[0]);
  EXPECT_DOUBLE_EQ(22377, buffer[4]);
  EXPECT_DOUBLE_EQ(16389, buffer[7]);
  EXPECT_DOUBLE_EQ(17631, buffer[kBufferLength - 1]);

  file->Rewind();

  // The next test is for checking the case where there are not as much data as
  // needed in the file, but reads to the end, and it returns the number of
  // int16s read.
  const size_t kBufferLenghtLargerThanFile = kBufferLength * 2;
  buffer.reset(new float[kBufferLenghtLargerThanFile]);
  EXPECT_EQ(kBufferLength,
            ReadInt16FromFileToFloatBuffer(file.get(),
                                           kBufferLenghtLargerThanFile,
                                           buffer.get()));
  EXPECT_DOUBLE_EQ(11544, buffer[0]);
  EXPECT_DOUBLE_EQ(22377, buffer[4]);
  EXPECT_DOUBLE_EQ(16389, buffer[7]);
  EXPECT_DOUBLE_EQ(17631, buffer[kBufferLength - 1]);
}

TEST_F(TransientFileUtilsTest,
       DISABLED_ON_IOS(ReadInt16FromFileToDoubleBuffer)) {
  std::string test_filename = kTestFileName;

  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  file->OpenFile(test_filename.c_str(),
                 true,    // Read only.
                 true,    // Loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kTestFileName.c_str();

  const size_t kBufferLength = 12;
  rtc::scoped_ptr<double[]> buffer(new double[kBufferLength]);

  EXPECT_EQ(kBufferLength, ReadInt16FromFileToDoubleBuffer(file.get(),
                                                           kBufferLength,
                                                           buffer.get()));
  EXPECT_DOUBLE_EQ(11544, buffer[0]);
  EXPECT_DOUBLE_EQ(22377, buffer[4]);
  EXPECT_DOUBLE_EQ(16389, buffer[7]);
  EXPECT_DOUBLE_EQ(17631, buffer[kBufferLength - 1]);

  file->Rewind();

  // The next test is for checking the case where there are not as much data as
  // needed in the file, but reads to the end, and it returns the number of
  // int16s read.
  const size_t kBufferLenghtLargerThanFile = kBufferLength * 2;
  buffer.reset(new double[kBufferLenghtLargerThanFile]);
  EXPECT_EQ(kBufferLength,
            ReadInt16FromFileToDoubleBuffer(file.get(),
                                            kBufferLenghtLargerThanFile,
                                            buffer.get()));
  EXPECT_DOUBLE_EQ(11544, buffer[0]);
  EXPECT_DOUBLE_EQ(22377, buffer[4]);
  EXPECT_DOUBLE_EQ(16389, buffer[7]);
  EXPECT_DOUBLE_EQ(17631, buffer[kBufferLength - 1]);
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ReadFloatBufferFromFile)) {
  std::string test_filename = kTestFileNamef;

  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  file->OpenFile(test_filename.c_str(),
                 true,    // Read only.
                 true,    // Loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kTestFileNamef.c_str();

  const size_t kBufferLength = 3;
  rtc::scoped_ptr<float[]> buffer(new float[kBufferLength]);

  EXPECT_EQ(kBufferLength, ReadFloatBufferFromFile(file.get(),
                                                   kBufferLength,
                                                   buffer.get()));
  EXPECT_FLOAT_EQ(kPi, buffer[0]);
  EXPECT_FLOAT_EQ(kE, buffer[1]);
  EXPECT_FLOAT_EQ(kAvogadro, buffer[2]);

  file->Rewind();

  // The next test is for checking the case where there are not as much data as
  // needed in the file, but reads to the end, and it returns the number of
  // doubles read.
  const size_t kBufferLenghtLargerThanFile = kBufferLength * 2;
  buffer.reset(new float[kBufferLenghtLargerThanFile]);
  EXPECT_EQ(kBufferLength, ReadFloatBufferFromFile(file.get(),
                                                   kBufferLenghtLargerThanFile,
                                                   buffer.get()));
  EXPECT_FLOAT_EQ(kPi, buffer[0]);
  EXPECT_FLOAT_EQ(kE, buffer[1]);
  EXPECT_FLOAT_EQ(kAvogadro, buffer[2]);
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ReadDoubleBufferFromFile)) {
  std::string test_filename = kTestFileName;

  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  file->OpenFile(test_filename.c_str(),
                 true,    // Read only.
                 true,    // Loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kTestFileName.c_str();

  const size_t kBufferLength = 3;
  rtc::scoped_ptr<double[]> buffer(new double[kBufferLength]);

  EXPECT_EQ(kBufferLength, ReadDoubleBufferFromFile(file.get(),
                                                    kBufferLength,
                                                    buffer.get()));
  EXPECT_DOUBLE_EQ(kPi, buffer[0]);
  EXPECT_DOUBLE_EQ(kE, buffer[1]);
  EXPECT_DOUBLE_EQ(kAvogadro, buffer[2]);

  file->Rewind();

  // The next test is for checking the case where there are not as much data as
  // needed in the file, but reads to the end, and it returns the number of
  // doubles read.
  const size_t kBufferLenghtLargerThanFile = kBufferLength * 2;
  buffer.reset(new double[kBufferLenghtLargerThanFile]);
  EXPECT_EQ(kBufferLength, ReadDoubleBufferFromFile(file.get(),
                                                    kBufferLenghtLargerThanFile,
                                                    buffer.get()));
  EXPECT_DOUBLE_EQ(kPi, buffer[0]);
  EXPECT_DOUBLE_EQ(kE, buffer[1]);
  EXPECT_DOUBLE_EQ(kAvogadro, buffer[2]);
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(WriteInt16BufferToFile)) {
  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  std::string kOutFileName = test::TempFilename(test::OutputPath(),
                                                "utils_test");

  file->OpenFile(kOutFileName.c_str(),
                 false,   // Write mode.
                 false,   // No loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kOutFileName.c_str();

  const size_t kBufferLength = 3;
  rtc::scoped_ptr<int16_t[]> written_buffer(new int16_t[kBufferLength]);
  rtc::scoped_ptr<int16_t[]> read_buffer(new int16_t[kBufferLength]);

  written_buffer[0] = 1;
  written_buffer[1] = 2;
  written_buffer[2] = 3;

  EXPECT_EQ(kBufferLength, WriteInt16BufferToFile(file.get(),
                                                  kBufferLength,
                                                  written_buffer.get()));

  file->CloseFile();

  file->OpenFile(kOutFileName.c_str(),
                 true,    // Read only.
                 false,   // No loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kOutFileName.c_str();

  EXPECT_EQ(kBufferLength, ReadInt16BufferFromFile(file.get(),
                                                   kBufferLength,
                                                   read_buffer.get()));
  EXPECT_EQ(0, memcmp(written_buffer.get(),
                      read_buffer.get(),
                      kBufferLength * sizeof(written_buffer[0])));
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(WriteFloatBufferToFile)) {
  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  std::string kOutFileName = test::TempFilename(test::OutputPath(),
                                                "utils_test");

  file->OpenFile(kOutFileName.c_str(),
                 false,   // Write mode.
                 false,   // No loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kOutFileName.c_str();

  const size_t kBufferLength = 3;
  rtc::scoped_ptr<float[]> written_buffer(new float[kBufferLength]);
  rtc::scoped_ptr<float[]> read_buffer(new float[kBufferLength]);

  written_buffer[0] = kPi;
  written_buffer[1] = kE;
  written_buffer[2] = kAvogadro;

  EXPECT_EQ(kBufferLength, WriteFloatBufferToFile(file.get(),
                                                  kBufferLength,
                                                  written_buffer.get()));

  file->CloseFile();

  file->OpenFile(kOutFileName.c_str(),
                 true,    // Read only.
                 false,   // No loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kOutFileName.c_str();

  EXPECT_EQ(kBufferLength, ReadFloatBufferFromFile(file.get(),
                                                   kBufferLength,
                                                   read_buffer.get()));
  EXPECT_EQ(0, memcmp(written_buffer.get(),
                      read_buffer.get(),
                      kBufferLength * sizeof(written_buffer[0])));
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(WriteDoubleBufferToFile)) {
  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  std::string kOutFileName = test::TempFilename(test::OutputPath(),
                                                "utils_test");

  file->OpenFile(kOutFileName.c_str(),
                 false,   // Write mode.
                 false,   // No loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kOutFileName.c_str();

  const size_t kBufferLength = 3;
  rtc::scoped_ptr<double[]> written_buffer(new double[kBufferLength]);
  rtc::scoped_ptr<double[]> read_buffer(new double[kBufferLength]);

  written_buffer[0] = kPi;
  written_buffer[1] = kE;
  written_buffer[2] = kAvogadro;

  EXPECT_EQ(kBufferLength, WriteDoubleBufferToFile(file.get(),
                                                   kBufferLength,
                                                   written_buffer.get()));

  file->CloseFile();

  file->OpenFile(kOutFileName.c_str(),
                 true,    // Read only.
                 false,   // No loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kOutFileName.c_str();

  EXPECT_EQ(kBufferLength, ReadDoubleBufferFromFile(file.get(),
                                                    kBufferLength,
                                                    read_buffer.get()));
  EXPECT_EQ(0, memcmp(written_buffer.get(),
                      read_buffer.get(),
                      kBufferLength * sizeof(written_buffer[0])));
}

TEST_F(TransientFileUtilsTest, DISABLED_ON_IOS(ExpectedErrorReturnValues)) {
  std::string test_filename = kTestFileName;

  double value;
  rtc::scoped_ptr<int16_t[]> int16_buffer(new int16_t[1]);
  rtc::scoped_ptr<double[]> double_buffer(new double[1]);
  rtc::scoped_ptr<FileWrapper> file(FileWrapper::Create());

  EXPECT_EQ(-1, ConvertByteArrayToDouble(NULL, &value));
  EXPECT_EQ(-1, ConvertByteArrayToDouble(kPiBytes, NULL));

  EXPECT_EQ(-1, ConvertDoubleToByteArray(kPi, NULL));

  // Tests with file not opened.
  EXPECT_EQ(0u, ReadInt16BufferFromFile(file.get(), 1, int16_buffer.get()));
  EXPECT_EQ(0u, ReadInt16FromFileToDoubleBuffer(file.get(),
                                                1,
                                                double_buffer.get()));
  EXPECT_EQ(0u, ReadDoubleBufferFromFile(file.get(), 1, double_buffer.get()));
  EXPECT_EQ(0u, WriteInt16BufferToFile(file.get(), 1, int16_buffer.get()));
  EXPECT_EQ(0u, WriteDoubleBufferToFile(file.get(), 1, double_buffer.get()));

  file->OpenFile(test_filename.c_str(),
                 true,    // Read only.
                 true,    // Loop.
                 false);  // No text.
  ASSERT_TRUE(file->Open()) << "File could not be opened:\n"
      << kTestFileName.c_str();

  EXPECT_EQ(0u, ReadInt16BufferFromFile(NULL, 1, int16_buffer.get()));
  EXPECT_EQ(0u, ReadInt16BufferFromFile(file.get(), 1, NULL));
  EXPECT_EQ(0u, ReadInt16BufferFromFile(file.get(), 0, int16_buffer.get()));

  EXPECT_EQ(0u, ReadInt16FromFileToDoubleBuffer(NULL, 1, double_buffer.get()));
  EXPECT_EQ(0u, ReadInt16FromFileToDoubleBuffer(file.get(), 1, NULL));
  EXPECT_EQ(0u, ReadInt16FromFileToDoubleBuffer(file.get(),
                                                0,
                                                double_buffer.get()));

  EXPECT_EQ(0u, ReadDoubleBufferFromFile(NULL, 1, double_buffer.get()));
  EXPECT_EQ(0u, ReadDoubleBufferFromFile(file.get(), 1, NULL));
  EXPECT_EQ(0u, ReadDoubleBufferFromFile(file.get(), 0, double_buffer.get()));

  EXPECT_EQ(0u, WriteInt16BufferToFile(NULL, 1, int16_buffer.get()));
  EXPECT_EQ(0u, WriteInt16BufferToFile(file.get(), 1, NULL));
  EXPECT_EQ(0u, WriteInt16BufferToFile(file.get(), 0, int16_buffer.get()));

  EXPECT_EQ(0u, WriteDoubleBufferToFile(NULL, 1, double_buffer.get()));
  EXPECT_EQ(0u, WriteDoubleBufferToFile(file.get(), 1, NULL));
  EXPECT_EQ(0u, WriteDoubleBufferToFile(file.get(), 0, double_buffer.get()));
}

}  // namespace webrtc

