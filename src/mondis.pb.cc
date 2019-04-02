// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: mondis.proto

#include "mondis.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

namespace mondis {
class MessageDefaultTypeInternal {
 public:
  ::google::protobuf::internal::ExplicitlyConstructed<Message> _instance;
} _Message_default_instance_;
}  // namespace mondis
static void InitDefaultsMessage_mondis_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::mondis::_Message_default_instance_;
    new (ptr) ::mondis::Message();
    ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::mondis::Message::InitAsDefaultInstance();
}

::google::protobuf::internal::SCCInfo<0> scc_info_Message_mondis_2eproto =
    {{ATOMIC_VAR_INIT(::google::protobuf::internal::SCCInfoBase::kUninitialized), 0, InitDefaultsMessage_mondis_2eproto}, {}};

void InitDefaults_mondis_2eproto() {
  ::google::protobuf::internal::InitSCC(&scc_info_Message_mondis_2eproto.base);
}

::google::protobuf::Metadata file_level_metadata_mondis_2eproto[1];
const ::google::protobuf::EnumDescriptor* file_level_enum_descriptors_mondis_2eproto[4];
constexpr ::google::protobuf::ServiceDescriptor const** file_level_service_descriptors_mondis_2eproto = nullptr;

const ::google::protobuf::uint32 TableStruct_mondis_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::mondis::Message, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::mondis::Message, msg_type_),
  PROTOBUF_FIELD_OFFSET(::mondis::Message, content_),
  PROTOBUF_FIELD_OFFSET(::mondis::Message, res_type_),
  PROTOBUF_FIELD_OFFSET(::mondis::Message, data_type_),
  PROTOBUF_FIELD_OFFSET(::mondis::Message, command_type_),
};
static const ::google::protobuf::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::mondis::Message)},
};

static ::google::protobuf::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::google::protobuf::Message*>(&::mondis::_Message_default_instance_),
};

::google::protobuf::internal::AssignDescriptorsTable assign_descriptors_table_mondis_2eproto = {
  {}, AddDescriptors_mondis_2eproto, "mondis.proto", schemas,
  file_default_instances, TableStruct_mondis_2eproto::offsets,
  file_level_metadata_mondis_2eproto, 1, file_level_enum_descriptors_mondis_2eproto, file_level_service_descriptors_mondis_2eproto,
};

const char descriptor_table_protodef_mondis_2eproto[] =
  "\n\014mondis.proto\022\006mondis\"\264\001\n\007Message\022!\n\010ms"
  "g_type\030\001 \001(\0162\017.mondis.MsgType\022\017\n\007content"
  "\030\002 \001(\t\022%\n\010res_type\030\003 \001(\0162\023.mondis.ExecRe"
  "sType\022#\n\tdata_type\030\004 \001(\0162\020.mondis.DataTy"
  "pe\022)\n\014command_type\030\005 \001(\0162\023.mondis.Comman"
  "dType*L\n\013ExecResType\022\006\n\002OK\020\000\022\020\n\014SYNTAX_E"
  "RROR\020\001\022\022\n\016INTERNAL_ERROR\020\002\022\017\n\013LOGIC_ERRO"
  "R\020\003*.\n\007MsgType\022\010\n\004DATA\020\000\022\013\n\007COMMAND\020\001\022\014\n"
  "\010EXEC_RES\020\002*:\n\010DataType\022\r\n\tSYNC_DATA\020\000\022\017"
  "\n\013CONTROL_MSG\020\001\022\016\n\nHEART_BEAT\020\002*m\n\013Comma"
  "ndType\022\022\n\016CLIENT_COMMAND\020\000\022\022\n\016MASTER_COM"
  "MAND\020\001\022\020\n\014PEER_COMMAND\020\002\022\021\n\rSLAVE_FORWAR"
  "D\020\003\022\021\n\rTIMER_COMMAND\020\004b\006proto3"
  ;
::google::protobuf::internal::DescriptorTable descriptor_table_mondis_2eproto = {
  false, InitDefaults_mondis_2eproto, 
  descriptor_table_protodef_mondis_2eproto,
  "mondis.proto", &assign_descriptors_table_mondis_2eproto, 510,
};

void AddDescriptors_mondis_2eproto() {
  static constexpr ::google::protobuf::internal::InitFunc deps[1] =
  {
  };
 ::google::protobuf::internal::AddDescriptors(&descriptor_table_mondis_2eproto, deps, 0);
}

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_mondis_2eproto = []() { AddDescriptors_mondis_2eproto(); return true; }();
namespace mondis {
const ::google::protobuf::EnumDescriptor* ExecResType_descriptor() {
  ::google::protobuf::internal::AssignDescriptors(&assign_descriptors_table_mondis_2eproto);
  return file_level_enum_descriptors_mondis_2eproto[0];
}
bool ExecResType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* MsgType_descriptor() {
  ::google::protobuf::internal::AssignDescriptors(&assign_descriptors_table_mondis_2eproto);
  return file_level_enum_descriptors_mondis_2eproto[1];
}
bool MsgType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* DataType_descriptor() {
  ::google::protobuf::internal::AssignDescriptors(&assign_descriptors_table_mondis_2eproto);
  return file_level_enum_descriptors_mondis_2eproto[2];
}
bool DataType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* CommandType_descriptor() {
  ::google::protobuf::internal::AssignDescriptors(&assign_descriptors_table_mondis_2eproto);
  return file_level_enum_descriptors_mondis_2eproto[3];
}
bool CommandType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      return true;
    default:
      return false;
  }
}


// ===================================================================

void Message::InitAsDefaultInstance() {
}
class Message::HasBitSetters {
 public:
};

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int Message::kMsgTypeFieldNumber;
const int Message::kContentFieldNumber;
const int Message::kResTypeFieldNumber;
const int Message::kDataTypeFieldNumber;
const int Message::kCommandTypeFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

Message::Message()
  : ::google::protobuf::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:mondis.Message)
}
Message::Message(const Message& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(nullptr) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  content_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.content().size() > 0) {
    content_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.content_);
  }
  ::memcpy(&msg_type_, &from.msg_type_,
    static_cast<size_t>(reinterpret_cast<char*>(&command_type_) -
    reinterpret_cast<char*>(&msg_type_)) + sizeof(command_type_));
  // @@protoc_insertion_point(copy_constructor:mondis.Message)
}

void Message::SharedCtor() {
  ::google::protobuf::internal::InitSCC(
      &scc_info_Message_mondis_2eproto.base);
  content_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&msg_type_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&command_type_) -
      reinterpret_cast<char*>(&msg_type_)) + sizeof(command_type_));
}

Message::~Message() {
  // @@protoc_insertion_point(destructor:mondis.Message)
  SharedDtor();
}

void Message::SharedDtor() {
  content_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void Message::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const Message& Message::default_instance() {
  ::google::protobuf::internal::InitSCC(&::scc_info_Message_mondis_2eproto.base);
  return *internal_default_instance();
}


void Message::Clear() {
// @@protoc_insertion_point(message_clear_start:mondis.Message)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  content_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&msg_type_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&command_type_) -
      reinterpret_cast<char*>(&msg_type_)) + sizeof(command_type_));
  _internal_metadata_.Clear();
}

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
const char* Message::_InternalParse(const char* begin, const char* end, void* object,
                  ::google::protobuf::internal::ParseContext* ctx) {
  auto msg = static_cast<Message*>(object);
  ::google::protobuf::int32 size; (void)size;
  int depth; (void)depth;
  ::google::protobuf::uint32 tag;
  ::google::protobuf::internal::ParseFunc parser_till_end; (void)parser_till_end;
  auto ptr = begin;
  while (ptr < end) {
    ptr = ::google::protobuf::io::Parse32(ptr, &tag);
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    switch (tag >> 3) {
      // .mondis.MsgType msg_type = 1;
      case 1: {
        if (static_cast<::google::protobuf::uint8>(tag) != 8) goto handle_unusual;
        ::google::protobuf::uint64 val = ::google::protobuf::internal::ReadVarint(&ptr);
        msg->set_msg_type(static_cast<::mondis::MsgType>(val));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // string content = 2;
      case 2: {
        if (static_cast<::google::protobuf::uint8>(tag) != 18) goto handle_unusual;
        ptr = ::google::protobuf::io::ReadSize(ptr, &size);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        ctx->extra_parse_data().SetFieldName("mondis.Message.content");
        object = msg->mutable_content();
        if (size > end - ptr + ::google::protobuf::internal::ParseContext::kSlopBytes) {
          parser_till_end = ::google::protobuf::internal::GreedyStringParserUTF8;
          goto string_till_end;
        }
        GOOGLE_PROTOBUF_PARSER_ASSERT(::google::protobuf::internal::StringCheckUTF8(ptr, size, ctx));
        ::google::protobuf::internal::InlineGreedyStringParser(object, ptr, size, ctx);
        ptr += size;
        break;
      }
      // .mondis.ExecResType res_type = 3;
      case 3: {
        if (static_cast<::google::protobuf::uint8>(tag) != 24) goto handle_unusual;
        ::google::protobuf::uint64 val = ::google::protobuf::internal::ReadVarint(&ptr);
        msg->set_res_type(static_cast<::mondis::ExecResType>(val));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // .mondis.DataType data_type = 4;
      case 4: {
        if (static_cast<::google::protobuf::uint8>(tag) != 32) goto handle_unusual;
        ::google::protobuf::uint64 val = ::google::protobuf::internal::ReadVarint(&ptr);
        msg->set_data_type(static_cast<::mondis::DataType>(val));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // .mondis.CommandType command_type = 5;
      case 5: {
        if (static_cast<::google::protobuf::uint8>(tag) != 40) goto handle_unusual;
        ::google::protobuf::uint64 val = ::google::protobuf::internal::ReadVarint(&ptr);
        msg->set_command_type(static_cast<::mondis::CommandType>(val));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->EndGroup(tag);
          return ptr;
        }
        auto res = UnknownFieldParse(tag, {_InternalParse, msg},
          ptr, end, msg->_internal_metadata_.mutable_unknown_fields(), ctx);
        ptr = res.first;
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr != nullptr);
        if (res.second) return ptr;
      }
    }  // switch
  }  // while
  return ptr;
string_till_end:
  static_cast<::std::string*>(object)->clear();
  static_cast<::std::string*>(object)->reserve(size);
  goto len_delim_till_end;
len_delim_till_end:
  return ctx->StoreAndTailCall(ptr, end, {_InternalParse, msg},
                               {parser_till_end, object}, size);
}
#else  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
bool Message::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!PROTOBUF_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:mondis.Message)
  for (;;) {
    ::std::pair<::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // .mondis.MsgType msg_type = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (8 & 0xFF)) {
          int value = 0;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          set_msg_type(static_cast< ::mondis::MsgType >(value));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string content = 2;
      case 2: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (18 & 0xFF)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_content()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->content().data(), static_cast<int>(this->content().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "mondis.Message.content"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // .mondis.ExecResType res_type = 3;
      case 3: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (24 & 0xFF)) {
          int value = 0;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          set_res_type(static_cast< ::mondis::ExecResType >(value));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // .mondis.DataType data_type = 4;
      case 4: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (32 & 0xFF)) {
          int value = 0;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          set_data_type(static_cast< ::mondis::DataType >(value));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // .mondis.CommandType command_type = 5;
      case 5: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (40 & 0xFF)) {
          int value = 0;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          set_command_type(static_cast< ::mondis::CommandType >(value));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:mondis.Message)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:mondis.Message)
  return false;
#undef DO_
}
#endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER

void Message::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:mondis.Message)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // .mondis.MsgType msg_type = 1;
  if (this->msg_type() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->msg_type(), output);
  }

  // string content = 2;
  if (this->content().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->content().data(), static_cast<int>(this->content().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "mondis.Message.content");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->content(), output);
  }

  // .mondis.ExecResType res_type = 3;
  if (this->res_type() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      3, this->res_type(), output);
  }

  // .mondis.DataType data_type = 4;
  if (this->data_type() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      4, this->data_type(), output);
  }

  // .mondis.CommandType command_type = 5;
  if (this->command_type() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      5, this->command_type(), output);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        _internal_metadata_.unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:mondis.Message)
}

::google::protobuf::uint8* Message::InternalSerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:mondis.Message)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // .mondis.MsgType msg_type = 1;
  if (this->msg_type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->msg_type(), target);
  }

  // string content = 2;
  if (this->content().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->content().data(), static_cast<int>(this->content().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "mondis.Message.content");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->content(), target);
  }

  // .mondis.ExecResType res_type = 3;
  if (this->res_type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      3, this->res_type(), target);
  }

  // .mondis.DataType data_type = 4;
  if (this->data_type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      4, this->data_type(), target);
  }

  // .mondis.CommandType command_type = 5;
  if (this->command_type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      5, this->command_type(), target);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mondis.Message)
  return target;
}

size_t Message::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mondis.Message)
  size_t total_size = 0;

  if (_internal_metadata_.have_unknown_fields()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        _internal_metadata_.unknown_fields());
  }
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string content = 2;
  if (this->content().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->content());
  }

  // .mondis.MsgType msg_type = 1;
  if (this->msg_type() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::EnumSize(this->msg_type());
  }

  // .mondis.ExecResType res_type = 3;
  if (this->res_type() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::EnumSize(this->res_type());
  }

  // .mondis.DataType data_type = 4;
  if (this->data_type() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::EnumSize(this->data_type());
  }

  // .mondis.CommandType command_type = 5;
  if (this->command_type() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::EnumSize(this->command_type());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void Message::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:mondis.Message)
  GOOGLE_DCHECK_NE(&from, this);
  const Message* source =
      ::google::protobuf::DynamicCastToGenerated<Message>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:mondis.Message)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:mondis.Message)
    MergeFrom(*source);
  }
}

void Message::MergeFrom(const Message& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:mondis.Message)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.content().size() > 0) {

    content_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.content_);
  }
  if (from.msg_type() != 0) {
    set_msg_type(from.msg_type());
  }
  if (from.res_type() != 0) {
    set_res_type(from.res_type());
  }
  if (from.data_type() != 0) {
    set_data_type(from.data_type());
  }
  if (from.command_type() != 0) {
    set_command_type(from.command_type());
  }
}

void Message::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:mondis.Message)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Message::CopyFrom(const Message& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mondis.Message)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Message::IsInitialized() const {
  return true;
}

void Message::Swap(Message* other) {
  if (other == this) return;
  InternalSwap(other);
}
void Message::InternalSwap(Message* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  content_.Swap(&other->content_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  swap(msg_type_, other->msg_type_);
  swap(res_type_, other->res_type_);
  swap(data_type_, other->data_type_);
  swap(command_type_, other->command_type_);
}

::google::protobuf::Metadata Message::GetMetadata() const {
  ::google::protobuf::internal::AssignDescriptors(&::assign_descriptors_table_mondis_2eproto);
  return ::file_level_metadata_mondis_2eproto[kIndexInFileMessages];
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace mondis
namespace google {
namespace protobuf {
template<> PROTOBUF_NOINLINE ::mondis::Message* Arena::CreateMaybeMessage< ::mondis::Message >(Arena* arena) {
  return Arena::CreateInternal< ::mondis::Message >(arena);
}
}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
