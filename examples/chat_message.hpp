#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <string>
#include <vector>
#include <boost/system/error_code.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/string_param.hpp>

namespace chat{

// chat protocol format
//           "inv:3:'message'
//                  'message'
//                  'message'"

// Message format
//         "message:length:nickname:string"
// example "message:14:Arnold:Hello World!!!"
struct Message{

    static constexpr std::size_t prefix_length = 7;
    static constexpr char const* prefix_string = "message";

    std::string payload_;
    std::string nickname_;

    Message()
    {}

    Message(const std::string & payload, const std::string & nickname)
        : payload_{payload}, nickname_{nickname}
    {}

    std::size_t get_serial_size() const{
        return prefix_length + std::to_string(payload_.size()).size() + nickname_.size() + payload_.size() + 3;
    }

    auto serialize(std::string & out) const{
        boost::beast::string_view::size_type write_bytes = 0;

        out.append(prefix_string);
        write_bytes += prefix_length;

        out.append(":");
        write_bytes += 1;

        out.append(std::to_string(payload_.size()));
        write_bytes += std::to_string(payload_.size()).size();

        out.append(":");
        write_bytes += 1;

        out.append(nickname_);
        write_bytes += nickname_.size();

        out.append(":");
        write_bytes += 1;

        out.append(payload_);
        write_bytes += payload_.size();

        return write_bytes;
    }

    auto parse(const std::string & in, boost::system::error_code & ec){
        std::string payload, nickname;

        boost::beast::string_view::size_type read_bytes = 0;
        boost::beast::string_view copy_in = in;

        if(copy_in.size() < prefix_length + 1){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        if(copy_in.substr(0, prefix_length) != boost::beast::string_view{prefix_string, prefix_length}){
            ec = boost::system::error_code{boost::system::errc::invalid_argument,
                    boost::system::system_category()};
            return read_bytes;
        }

        read_bytes += prefix_length;

        if(copy_in.at(prefix_length) != ':'){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        read_bytes += 1;

        copy_in = copy_in.substr(prefix_length + 1);

        boost::beast::string_view::size_type pos_end_length;
        if((pos_end_length = copy_in.find_first_of(':')) == boost::beast::string_view::npos){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        uint32_t pay_sz = static_cast<uint32_t>(std::atoi(copy_in.substr(0, pos_end_length).data()));
        if(pay_sz == 0){
            ec = boost::system::error_code{boost::system::errc::invalid_argument,
                    boost::system::system_category()};
            return read_bytes;
        }

        read_bytes += pos_end_length + 1;

        copy_in = copy_in.substr(pos_end_length + 1);

        boost::beast::string_view::size_type pos_end_nickname;
        if((pos_end_nickname = copy_in.find_first_of(':')) == boost::beast::string_view::npos){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        nickname = copy_in.substr(0, pos_end_nickname).to_string();

        read_bytes += pos_end_nickname + 1;

        copy_in = copy_in.substr(pos_end_nickname + 1);

        payload = copy_in.substr(0, static_cast<boost::beast::string_view::size_type>(pay_sz)).to_string();

        read_bytes += pay_sz;

        std::swap(payload_, payload);
        std::swap(nickname_, nickname);

        ec = boost::system::error_code{boost::system::errc::success, boost::system::system_category()};
        return read_bytes;
    }

}; // Message class

std::size_t get_serial_size(const Message & m){
    return m.get_serial_size();
};

struct Inv{

    static constexpr const char* prefix_string = "inv";
    static constexpr const std::size_t prefix_length = 3;

    uint32_t count_entry_;

    Inv()
    {}

    Inv(uint32_t count_entry)
        : count_entry_{count_entry}
    {}

    std::size_t get_serial_size() const{
        return prefix_length + std::to_string(count_entry_).size() + 2;
    }

    auto serialize(std::string & out) const{
        boost::beast::string_view::size_type write_bytes = 0;

        out.append(prefix_string);
        write_bytes += prefix_length;

        out.append(":");
        write_bytes += 1;

        out.append(std::to_string(count_entry_));
        write_bytes += std::to_string(count_entry_).size();

        out.append(":");
        write_bytes += 1;

        return write_bytes;
    }

    auto parse(const std::string & in, boost::system::error_code & ec){
        std::string payload, nickname;

        boost::beast::string_view::size_type read_bytes = 0;
        boost::beast::string_view copy_in = in;

        if(copy_in.size() < prefix_length + 1){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        if(copy_in.substr(0, prefix_length) != boost::beast::string_view{prefix_string, prefix_length}){
            ec = boost::system::error_code{boost::system::errc::invalid_argument,
                    boost::system::system_category()};
            return read_bytes;
        }

        read_bytes += prefix_length;

        if(copy_in.at(prefix_length) != ':'){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        read_bytes += 1;

        copy_in = copy_in.substr(prefix_length + 1);

        boost::beast::string_view::size_type pos_end_length;
        if((pos_end_length = copy_in.find_first_of(':')) == boost::beast::string_view::npos){
            ec = boost::system::error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
            return read_bytes;
        }

        uint32_t count_entry = static_cast<uint32_t>(std::atoi(copy_in.substr(0, pos_end_length).data()));
        if(count_entry == 0){
            ec = boost::system::error_code{boost::system::errc::invalid_argument,
                    boost::system::system_category()};
            return read_bytes;
        }

        std::swap(count_entry_, count_entry);

        read_bytes += pos_end_length + 1;

        return read_bytes;
    }

};

template<class C, class T>
class Parser;

template<>
class Parser<Inv, Message>{

    std::vector<Message>& messages_;

public:

    Parser(std::vector<Message>& messages)
        : messages_{messages}
    {}

    auto advance(const std::string & in, boost::system::error_code & ec) const{
        std::size_t used_bytes = 0;

        messages_.clear();

        Inv i;
        used_bytes += i.parse(in, ec);

        if(ec)
            return used_bytes;

        while(i.count_entry_ > messages_.size()){
            Message m;
            used_bytes += m.parse(in.substr(used_bytes), ec);
            if(ec)
                return used_bytes;
            messages_.push_back(m);
        }
        return used_bytes;
    }

}; // Parser class

template<class C, class T>
class Serializer;

template<>
class Serializer<Inv, Message>{

    std::string& out_;

public:

    Serializer(std::string & out)
        : out_{out}
    {}

    auto advance(const std::vector<Message>& messages) const{
        std::size_t used_bytes = 0;

        if(messages.size() >= UINT32_MAX)
            throw std::runtime_error("messages.size() >= UINT32_MAX");

        auto const i = Inv{static_cast<uint32_t>(messages.size())};

        used_bytes += i.serialize(out_);

        for(auto & message : messages){
            used_bytes += message.serialize(out_);
        }
        return used_bytes;
    }

}; // Serializer class

} // namespace chat

#endif // CHAT_MESSAGE_HPP
