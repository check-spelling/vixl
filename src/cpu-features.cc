// Copyright 2018, VIXL authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <ostream>

#include "cpu-features.h"
#include "globals-vixl.h"
#include "utils-vixl.h"

namespace vixl {

static uint64_t MakeFeatureMask(CPUFeatures::Feature feature) {
  if (feature == CPUFeatures::kNone) {
    return 0;
  } else {
    // Check that the shift is well-defined, and that the feature is valid.
    VIXL_STATIC_ASSERT(CPUFeatures::kNumberOfFeatures <=
                       (sizeof(uint64_t) * 8));
    VIXL_ASSERT(feature < CPUFeatures::kNumberOfFeatures);
    return UINT64_C(1) << feature;
  }
}

CPUFeatures::CPUFeatures(Feature feature0,
                         Feature feature1,
                         Feature feature2,
                         Feature feature3)
    : features_(0) {
  Combine(feature0, feature1, feature2, feature3);
}

CPUFeatures CPUFeatures::All() {
  CPUFeatures all;
  // Check that the shift is well-defined.
  VIXL_STATIC_ASSERT(CPUFeatures::kNumberOfFeatures < (sizeof(uint64_t) * 8));
  all.features_ = (UINT64_C(1) << kNumberOfFeatures) - 1;
  return all;
}

CPUFeatures CPUFeatures::InferFromOS() {
  // TODO: Actually infer features from the OS.
  return CPUFeatures();
}

void CPUFeatures::Combine(const CPUFeatures& other) {
  features_ |= other.features_;
}

void CPUFeatures::Combine(Feature feature0,
                          Feature feature1,
                          Feature feature2,
                          Feature feature3) {
  features_ |= MakeFeatureMask(feature0);
  features_ |= MakeFeatureMask(feature1);
  features_ |= MakeFeatureMask(feature2);
  features_ |= MakeFeatureMask(feature3);
}

void CPUFeatures::Remove(const CPUFeatures& other) {
  features_ &= ~other.features_;
}

void CPUFeatures::Remove(Feature feature0,
                         Feature feature1,
                         Feature feature2,
                         Feature feature3) {
  features_ &= ~MakeFeatureMask(feature0);
  features_ &= ~MakeFeatureMask(feature1);
  features_ &= ~MakeFeatureMask(feature2);
  features_ &= ~MakeFeatureMask(feature3);
}

CPUFeatures CPUFeatures::With(const CPUFeatures& other) const {
  CPUFeatures f(*this);
  f.Combine(other);
  return f;
}

CPUFeatures CPUFeatures::With(Feature feature0,
                              Feature feature1,
                              Feature feature2,
                              Feature feature3) const {
  CPUFeatures f(*this);
  f.Combine(feature0, feature1, feature2, feature3);
  return f;
}

CPUFeatures CPUFeatures::Without(const CPUFeatures& other) const {
  CPUFeatures f(*this);
  f.Remove(other);
  return f;
}

CPUFeatures CPUFeatures::Without(Feature feature0,
                                 Feature feature1,
                                 Feature feature2,
                                 Feature feature3) const {
  CPUFeatures f(*this);
  f.Remove(feature0, feature1, feature2, feature3);
  return f;
}

bool CPUFeatures::Has(const CPUFeatures& other) const {
  return (features_ & other.features_) == other.features_;
}

bool CPUFeatures::Has(Feature feature0,
                      Feature feature1,
                      Feature feature2,
                      Feature feature3) const {
  uint64_t mask = MakeFeatureMask(feature0) | MakeFeatureMask(feature1) |
                  MakeFeatureMask(feature2) | MakeFeatureMask(feature3);
  return (features_ & mask) == mask;
}

size_t CPUFeatures::Count() const { return CountSetBits(features_); }

std::ostream& operator<<(std::ostream& os, CPUFeatures::Feature feature) {
  // clang-format off
  switch (feature) {
#define VIXL_FORMAT_FEATURE(SYMBOL, NAME, CPUINFO) \
    case CPUFeatures::SYMBOL:                      \
      return os << NAME;
VIXL_CPU_FEATURE_LIST(VIXL_FORMAT_FEATURE)
#undef VIXL_FORMAT_FEATURE
    case CPUFeatures::kNone:
      return os << "none";
  }
  // clang-format on
  VIXL_UNREACHABLE();
  return os;
}

CPUFeatures::const_iterator CPUFeatures::begin() const {
  if (features_ == 0) return const_iterator(this, kNone);

  int feature_number = CountTrailingZeros(features_);
  vixl::CPUFeatures::Feature feature =
      static_cast<CPUFeatures::Feature>(feature_number);
  return const_iterator(this, feature);
}

CPUFeatures::const_iterator CPUFeatures::end() const {
  return const_iterator(this, kNone);
}

std::ostream& operator<<(std::ostream& os, const CPUFeatures& features) {
  CPUFeatures::const_iterator it = features.begin();
  while (it != features.end()) {
    os << *it;
    ++it;
    if (it != features.end()) os << ", ";
  }
  return os;
}

bool CPUFeaturesConstIterator::operator==(
    const CPUFeaturesConstIterator& other) const {
  VIXL_ASSERT(IsValid());
  return (cpu_features_ == other.cpu_features_) && (feature_ == other.feature_);
}

CPUFeatures::Feature CPUFeaturesConstIterator::operator++() {  // Prefix
  VIXL_ASSERT(IsValid());
  do {
    // Find the next feature. The order is unspecified.
    VIXL_STATIC_ASSERT(CPUFeatures::kNone == CPUFeatures::kNumberOfFeatures);
    if (feature_ == CPUFeatures::kNone) {
      feature_ = static_cast<CPUFeatures::Feature>(0);
    } else {
      feature_ = static_cast<CPUFeatures::Feature>(feature_ + 1);
    }
    // cpu_features_->Has(kNone) is always true, so this will terminate even if
    // the features list is empty.
  } while (!cpu_features_->Has(feature_));
  return feature_;
}

CPUFeatures::Feature CPUFeaturesConstIterator::operator++(int) {  // Postfix
  CPUFeatures::Feature result = feature_;
  ++(*this);
  return result;
}

}  // namespace vixl
