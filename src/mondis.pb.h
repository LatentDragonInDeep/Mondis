// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: mondis.proto

#ifndef PROTOBUF_INCLUDED_mondis_2eproto
#define PROTOBUF_INCLUDED_mondis_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3007000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3007000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_mondis_2eproto

// Internal implementation detail -- do not use these members.
struct TableStruct_mondis_2eproto {
  static const ::google::protobuf::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors_mondis_2eproto();
namespace mondis {
class Message;
class MessageDefaultTypeInternal;
extern MessageDefaultTypeInternal _Message_default_instance_;
}  // namespace mondis
namespace google {
namespace protobuf {
template<> ::mondis::Message* Arena::CreateMaybeMessage<::mondis::Message>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace mondis {

enum ExecResType {
  OK = 0,
  SYNTAX_ERROR = 1,
  INTERNAL_ERROR = 2,
  LOGIC_ERROR = 3,
  ExecResType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  ExecResType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool ExecResType_IsValid(int value);
const ExecResType ExecResType_MIN = OK;
const ExecResType ExecResType_MAX = LOGIC_ERROR;
const int ExecResType_ARRAYSIZE = ExecResType_MAX + 1;

const ::google::protobuf::EnumDescriptor* ExecResType_descriptor();
inline const ::std::string& ExecResType_Name(ExecResType value) {
  return ::google::protobuf::internal::NameOfEnum(
    ExecResType_descriptor(), value);
}
inline bool ExecResType_Parse(
    const ::std::string& name, ExecResType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ExecResType>(
    ExecResType_descriptor(), name, value);
}
enum MsgType {
  DATA = 0,
  COMMAND = 1,
  EXEC_RES = 2,
  MsgType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  MsgType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool MsgType_IsValid(int value);
const MsgType MsgType_MIN = DATA;
const MsgType MsgType_MAX = EXEC_RES;
const int MsgType_ARRAYSIZE = MsgType_MAX + 1;

const ::google::protobuf::EnumDescriptor* MsgType_descriptor();
inline const ::std::string& MsgType_Name(MsgType value) {
  return ::google::protobuf::internal::NameOfEnum(
    MsgType_descriptor(), value);
}
inline bool MsgType_Parse(
    const ::std::string& name, MsgType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<MsgType>(
    MsgType_descriptor(), name, value);
}
enum DataType {
  SYNC_DATA = 0,
  CONTROL_MSG = 1,
  HEART_BEAT = 2,
  DataType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  DataType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool DataType_IsValid(int value);
const DataType DataType_MIN = SYNC_DATA;
const DataType DataType_MAX = HEART_BEAT;
const int DataType_ARRAYSIZE = DataType_MAX + 1;

const ::google::protobuf::EnumDescriptor* DataType_descriptor();
inline const ::std::string& DataType_Name(DataType value) {
  return ::google::protobuf::internal::NameOfEnum(
    DataType_descriptor(), value);
}
inline bool DataType_Parse(
    const ::std::string& name, DataType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<DataType>(
    DataType_descriptor(), name, value);
}
enum CommandType {
  CLIENT_COMMAND = 0,
  MASTER_COMMAND = 1,
  PEER_COMMAND = 2,
  SLAVE_FORWARD = 3,
  TIMER_COMMAND = 4,
  CommandType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  CommandType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool CommandType_IsValid(int value);
const CommandType CommandType_MIN = CLIENT_COMMAND;
const CommandType CommandType_MAX = TIMER_COMMAND;
const int CommandType_ARRAYSIZE = CommandType_MAX + 1;

const ::google::protobuf::EnumDescriptor* CommandType_descriptor();
inline const ::std::string& CommandType_Name(CommandType value) {
  return ::google::protobuf::internal::NameOfEnum(
    CommandType_descriptor(), value);
}
inline bool CommandType_Parse(
    const ::std::string& name, CommandType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<CommandType>(
    CommandType_descriptor(), name, value);
}
enum SendToType {
  ALL_CLIENTS = 0,
  ALL_PEERS = 1,
  SPECIFY_CLIENT = 2,
  SPECIFY_PEER = 3,
  NO_ONE = 4,
  SendToType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::min(),
  SendToType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::google::protobuf::int32>::max()
};
bool SendToType_IsValid(int value);
const SendToType SendToType_MIN = ALL_CLIENTS;
const SendToType SendToType_MAX = NO_ONE;
const int SendToType_ARRAYSIZE = SendToType_MAX + 1;

const ::google::protobuf::EnumDescriptor* SendToType_descriptor();
inline const ::std::string& SendToType_Name(SendToType value) {
  return ::google::protobuf::internal::NameOfEnum(
    SendToType_descriptor(), value);
}
inline bool SendToType_Parse(
    const ::std::string& name, SendToType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<SendToType>(
    SendToType_descriptor(), name, value);
}
// ===================================================================

class Message final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:mondis.Message) */ {
 public:
  Message();
  virtual ~Message();

  Message(const Message& from);

  inline Message& operator=(const Message& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  Message(Message&& from) noexcept
    : Message() {
    *this = ::std::move(from);
  }

  inline Message& operator=(Message&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor() {
    return default_instance().GetDescriptor();
  }
  static const Message& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Message* internal_default_instance() {
    return reinterpret_cast<const Message*>(
               &_Message_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(Message* other);
  friend void swap(Message& a, Message& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline Message* New() const final {
    return CreateMaybeMessage<Message>(nullptr);
  }

  Message* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<Message>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const Message& from);
  void MergeFrom(const Message& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  #if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  static const char* _InternalParse(const char* begin, const char* end, void* object, ::google::protobuf::internal::ParseContext* ctx);
  ::google::protobuf::internal::ParseFunc _ParseFunc() const final { return _InternalParse; }
  #else
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  #endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Message* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string content = 2;
  void clear_content();
  static const int kContentFieldNumber = 2;
  const ::std::string& content() const;
  void set_content(const ::std::string& value);
  #if LANG_CXX11
  void set_content(::std::string&& value);
  #endif
  void set_content(const char* value);
  void set_content(const char* value, size_t size);
  ::std::string* mutable_content();
  ::std::string* release_content();
  void set_allocated_content(::std::string* content);

  // .mondis.MsgType msg_type = 1;
  void clear_msg_type();
  static const int kMsgTypeFieldNumber = 1;
  ::mondis::MsgType msg_type() const;
  void set_msg_type(::mondis::MsgType value);

  // .mondis.ExecResType res_type = 3;
  void clear_res_type();
  static const int kResTypeFieldNumber = 3;
  ::mondis::ExecResType res_type() const;
  void set_res_type(::mondis::ExecResType value);

  // .mondis.DataType data_type = 4;
  void clear_data_type();
  static const int kDataTypeFieldNumber = 4;
  ::mondis::DataType data_type() const;
  void set_data_type(::mondis::DataType value);

  // .mondis.CommandType command_type = 5;
  void clear_command_type();
  static const int kCommandTypeFieldNumber = 5;
  ::mondis::CommandType command_type() const;
  void set_command_type(::mondis::CommandType value);

  // .mondis.SendToType send_to_type = 6;
  void clear_send_to_type();
  static const int kSendToTypeFieldNumber = 6;
  ::mondis::SendToType send_to_type() const;
  void set_send_to_type(::mondis::SendToType value);

  // uint32 client_id = 7;
  void clear_client_id();
  static const int kClientIdFieldNumber = 7;
  ::google::protobuf::uint32 client_id() const;
  void set_client_id(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:mondis.Message)
 private:
  class HasBitSetters;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr content_;
  int msg_type_;
  int res_type_;
  int data_type_;
  int command_type_;
  int send_to_type_;
  ::google::protobuf::uint32 client_id_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_mondis_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Message

// .mondis.MsgType msg_type = 1;
inline void Message::clear_msg_type() {
  msg_type_ = 0;
}
inline ::mondis::MsgType Message::msg_type() const {
  // @@protoc_insertion_point(field_get:mondis.Message.msg_type)
  return static_cast< ::mondis::MsgType >(msg_type_);
}
inline void Message::set_msg_type(::mondis::MsgType value) {
  
  msg_type_ = value;
  // @@protoc_insertion_point(field_set:mondis.Message.msg_type)
}

// string content = 2;
inline void Message::clear_content() {
  content_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& Message::content() const {
  // @@protoc_insertion_point(field_get:mondis.Message.content)
  return content_.GetNoArena();
}
inline void Message::set_content(const ::std::string& value) {
  
  content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:mondis.Message.content)
}
#if LANG_CXX11
inline void Message::set_content(::std::string&& value) {
  
  content_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:mondis.Message.content)
}
#endif
inline void Message::set_content(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:mondis.Message.content)
}
inline void Message::set_content(const char* value, size_t size) {
  
  content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:mondis.Message.content)
}
inline ::std::string* Message::mutable_content() {
  
  // @@protoc_insertion_point(field_mutable:mondis.Message.content)
  return content_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* Message::release_content() {
  // @@protoc_insertion_point(field_release:mondis.Message.content)
  
  return content_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void Message::set_allocated_content(::std::string* content) {
  if (content != nullptr) {
    
  } else {
    
  }
  content_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), content);
  // @@protoc_insertion_point(field_set_allocated:mondis.Message.content)
}

// .mondis.ExecResType res_type = 3;
inline void Message::clear_res_type() {
  res_type_ = 0;
}
inline ::mondis::ExecResType Message::res_type() const {
  // @@protoc_insertion_point(field_get:mondis.Message.res_type)
  return static_cast< ::mondis::ExecResType >(res_type_);
}
inline void Message::set_res_type(::mondis::ExecResType value) {
  
  res_type_ = value;
  // @@protoc_insertion_point(field_set:mondis.Message.res_type)
}

// .mondis.DataType data_type = 4;
inline void Message::clear_data_type() {
  data_type_ = 0;
}
inline ::mondis::DataType Message::data_type() const {
  // @@protoc_insertion_point(field_get:mondis.Message.data_type)
  return static_cast< ::mondis::DataType >(data_type_);
}
inline void Message::set_data_type(::mondis::DataType value) {
  
  data_type_ = value;
  // @@protoc_insertion_point(field_set:mondis.Message.data_type)
}

// .mondis.CommandType command_type = 5;
inline void Message::clear_command_type() {
  command_type_ = 0;
}
inline ::mondis::CommandType Message::command_type() const {
  // @@protoc_insertion_point(field_get:mondis.Message.command_type)
  return static_cast< ::mondis::CommandType >(command_type_);
}
inline void Message::set_command_type(::mondis::CommandType value) {
  
  command_type_ = value;
  // @@protoc_insertion_point(field_set:mondis.Message.command_type)
}

// .mondis.SendToType send_to_type = 6;
inline void Message::clear_send_to_type() {
  send_to_type_ = 0;
}
inline ::mondis::SendToType Message::send_to_type() const {
  // @@protoc_insertion_point(field_get:mondis.Message.send_to_type)
  return static_cast< ::mondis::SendToType >(send_to_type_);
}
inline void Message::set_send_to_type(::mondis::SendToType value) {
  
  send_to_type_ = value;
  // @@protoc_insertion_point(field_set:mondis.Message.send_to_type)
}

// uint32 client_id = 7;
inline void Message::clear_client_id() {
  client_id_ = 0u;
}
inline ::google::protobuf::uint32 Message::client_id() const {
  // @@protoc_insertion_point(field_get:mondis.Message.client_id)
  return client_id_;
}
inline void Message::set_client_id(::google::protobuf::uint32 value) {
  
  client_id_ = value;
  // @@protoc_insertion_point(field_set:mondis.Message.client_id)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace mondis

namespace google {
namespace protobuf {

template <> struct is_proto_enum< ::mondis::ExecResType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::mondis::ExecResType>() {
  return ::mondis::ExecResType_descriptor();
}
template <> struct is_proto_enum< ::mondis::MsgType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::mondis::MsgType>() {
  return ::mondis::MsgType_descriptor();
}
template <> struct is_proto_enum< ::mondis::DataType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::mondis::DataType>() {
  return ::mondis::DataType_descriptor();
}
template <> struct is_proto_enum< ::mondis::CommandType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::mondis::CommandType>() {
  return ::mondis::CommandType_descriptor();
}
template <> struct is_proto_enum< ::mondis::SendToType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::mondis::SendToType>() {
  return ::mondis::SendToType_descriptor();
}

}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // PROTOBUF_INCLUDED_mondis_2eproto