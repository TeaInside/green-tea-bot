import { SearchIcon } from "@heroicons/react/outline";
import Message from "./Message";

function ChatBox() {
    return (
        <div className="flex-grow h-screen bg-cream">
            <div className="flex items-center bg-white w-full pl-5 py-5 shadow-md sticky top-0">
                <img
                    className="w-12 h-12 rounded-full"
                    src="profile.jpeg"
                    alt=""
                />
                <div className="ml-4">
                    <h2 className="text-[20px] font-bold">Group 1</h2>
                    <p className="text-gray-400">Online</p>
                </div>
                <SearchIcon className="w-8 h-8 ml-auto mr-4 text-gray-400" />
            </div>
            <div className="space-y-6 flex flex-col  overflow-y-auto h-[87%] whitespace-nowrap p-4">
                <Message />
                <Message />
                <Message />
                <Message />
                <Message />
                <Message />
                <Message />
                <Message />
                <Message />
            </div>
        </div>
    );
}

export default ChatBox;
