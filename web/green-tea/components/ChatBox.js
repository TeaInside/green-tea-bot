import { SearchIcon, XIcon } from "@heroicons/react/outline";
import classNames from "classnames";
import { useState } from "react";
import Message from "./Message";

function ChatBox({ data }) {
    const [offcanvas, setOffcanvas] = useState(false);
    return (
        <div className="flex-grow flex  h-screen bg-cream">
            <div className={classNames("transition-all duration-150 ease-in-out", offcanvas ? "w-8/12" : "w-full")}>
                <div className="flex items-center bg-white w-full pl-5 h-[80px] shadow-md sticky top-0">
                    <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="" />
                    <div className="ml-4">
                        <h2 className="text-[20px] font-bold">Group 1</h2>
                        <p className="text-gray-400">Online</p>
                    </div>
                    <SearchIcon className="w-8 h-8 ml-auto mr-4 text-gray-400 cursor-pointer" onClick={() => setOffcanvas(true)} />
                </div>
                <div className="space-y-6 flex flex-col  overflow-y-scroll h-[89%] whitespace-nowrap p-4 scrollbar-thin scrollbar-thumb-gray-400">
                    {data.map(({ first_name, last_name, text }) => (
                        <Message first_name={first_name} last_name={last_name} text={text} />
                    ))}

                    {/* <Message />
                    <Message />
                    <Message />
                    <Message />
                    <Message />
                    <Message />
                    <Message />
                    <Message /> */}
                </div>
            </div>
            <div className={classNames("w-4/12 bg-white", offcanvas ? "block" : "hidden")}>
                <div className="flex items-center bg-white w-full pl-5 h-[80px] shadow-md sticky top-0">
                    <h1 className="text-[20px] text-gray-500">Search Message</h1>
                    <XIcon className="w-8 h-8 ml-auto mr-4 text-gray-400 cursor-pointer" onClick={() => setOffcanvas(false)} />
                </div>
                <div className="relative flex-grow my-4 rounded-lg mx-4">
                    <div className="absolute inset-y-0 flex items-center pl-3 pointer-events-none">
                        <SearchIcon className="h-5 w-5 text-gray-500" />
                    </div>
                    <input className="bg-gray-200 w-full h-10 pl-10 rounded-lg outline-none" type="text" placeholder="Search message" />
                </div>
                <div className="space-y-6 flex flex-col  overflow-y-auto h-[77%] whitespace-nowrap p-4">
                    <p className="text-center text-gray-400">Cari Pesan di Group 1</p>
                </div>
            </div>
        </div>
    );
}

export default ChatBox;
