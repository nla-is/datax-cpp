//
// Created by Giuseppe Coviello on 6/16/23.
//

#include "implementation.h"

#include <fstream>
#include <sstream>
#include <future>

using datax::sdk::implementation::Implementation;

int64_t datax::sdk::implementation::now() {
  return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string datax::sdk::implementation::Getenv(const std::string &variable) {
  auto value = getenv(variable.c_str());
  if (value == nullptr) {
    return "";
  }
  return {value};
}

void Implementation::report() {
  if (now() - latestReport < 10000) {
    return;
  }
  if (latestReport > 0) {
    auto elapsedTime = now() - latestReport;

    auto receiving = receivingTime - previousReceivingTime;
    auto decoding = decodingTime - previousDecodingTime;
    auto encoding = encodingTime - previousEncodingTime;
    auto transferring = transferringTime - previousTransferringTime;

    auto processing = elapsedTime - receiving - decoding - encoding - transferring;
    fprintf(stderr,
            "[DataX] Time for receiving: %ld ms (%0.2lf%%), decoding: %ld ms (%0.2lf%%), processing: %ld ms (%0.2lf%%), encoding: %ld ms (%0.2lf%%), transferring: %ld ms (%0.2lf%%)\n",
            receiving,
            (((double) receiving) / ((double) elapsedTime)) * 100,
            decoding,
            (((double) decoding) / ((double) elapsedTime)) * 100,
            processing,
            (((double) processing) / ((double) elapsedTime)) * 100,
            encoding,
            (((double) encoding) / ((double) elapsedTime)) * 100,
            transferring,
            (((double) transferring) / ((double) elapsedTime)) * 100
    );
  }

  assert(receivingTime >= previousReceivingTime);
  previousReceivingTime = receivingTime;
  assert(decodingTime >= previousDecodingTime);
  previousDecodingTime = decodingTime;
  assert(encodingTime >= previousEncodingTime);
  previousEncodingTime = encodingTime;
  assert(transferringTime >= previousTransferringTime);
  previousTransferringTime = transferringTime;

  latestReport = now();
}

Implementation::Implementation(std::initializer_list<Option> options) : receivingTime(0), decodingTime(0),
                                                                        encodingTime(0), transferringTime(0),
                                                                        previousReceivingTime(0), previousDecodingTime(0),
                                                                        previousEncodingTime(0), previousTransferringTime(0),
                                                                        latestReport(0) {
  auto sidecarAddress = Getenv("DATAX_SIDECAR_ADDRESS");
  if (sidecarAddress.empty()) {
    sidecarAddress = "127.0.0.1:20001";
  }
  grpc::ChannelArguments channelArguments;
  channelArguments.SetMaxSendMessageSize(32 * 1024 * 1024);
  channelArguments.SetMaxReceiveMessageSize(32 * 1024 * 1024);
  clientConn = grpc::CreateCustomChannel(sidecarAddress, grpc::InsecureChannelCredentials(), channelArguments);
  client = datax::sdk::protocol::v1::DataX::NewStub(clientConn);
  grpc::ClientContext context;
  datax::sdk::protocol::v1::Settings settings;
  auto status = client->Initialize(&context, settings, &initialization);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }

  for (auto & option : options) {
    option.Apply(this);
  }
}

nlohmann::json Implementation::Configuration() {
  auto configurationPath = Getenv("DATAX_CONFIGURATION");
  if (configurationPath.empty()) {
    configurationPath = "/datax/configuration";
  }
  std::ifstream in(configurationPath);
  return nlohmann::json::parse(in);
}

datax::RawMessage Implementation::NextRaw() {
  auto start = now();
  grpc::ClientContext context;
  datax::sdk::protocol::v1::NextOptions request;
  datax::sdk::protocol::v1::NextMessage reply;
  auto status = client->Next(&context, request, &reply);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }
  datax::RawMessage message;

  const auto &data = reply.data();
  if (data.empty()) {
    fprintf(stderr, "Empty data\n");
    exit(-1);
  }
  message.Data = std::vector<unsigned char>(reinterpret_cast<const unsigned char *>(data.data()),
                                            reinterpret_cast<const unsigned char *>(data.data()) + data.size());

  message.Reference = reply.reference();
  message.Stream = reply.stream();
  receivingTime += now() - start;
  report();
  return message;
}

datax::Message Implementation::Next() {
  auto raw = NextRaw();
  auto start = now();
  datax::Message msg;
  msg.Stream = raw.Stream;
  msg.Reference = raw.Reference;
  msg.Data = nlohmann::json::from_msgpack(raw.Data);
  decodingTime += now() - start;
  return msg;
}

void Implementation::Emit(const nlohmann::json &message, const std::string &reference) {
  auto start = now();
  auto data = nlohmann::json::to_msgpack(message);
  encodingTime += now() - start;
  EmitRaw(data, reference);
}

void Implementation::EmitRaw(const std::vector<unsigned char> &data, const std::string &reference) {
  EmitRaw(data.data(), static_cast<int>(data.size()), reference);
}

void Implementation::EmitRaw(const unsigned char *data, int dataSize, const std::string &reference) {
  auto start = now();
  grpc::ClientContext context;
  datax::sdk::protocol::v1::EmitMessage request;
  datax::sdk::protocol::v1::EmitResult reply;

  request.set_data(data, dataSize);
  request.set_reference(reference);

  auto status = client->Emit(&context, request, &reply);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }
  transferringTime += now() - start;
  report();
}

std::vector<nlohmann::json> Implementation::FanOut(const std::vector<nlohmann::json> &messages) {
  auto a = std::async([&]() {
    for (auto &msg: messages) {
      fanOutInputQueue->push(msg);
    }
  });
  std::vector<nlohmann::json> result;
  while (result.size() < messages.size()) {
    result.emplace_back(fanOutOutputQueue->pop());
  }
  a.wait();
  return result;
}
