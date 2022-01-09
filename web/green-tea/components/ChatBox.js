import { SearchIcon, XIcon } from "@heroicons/react/outline";
import classNames from "classnames";
import { useState } from "react";
import Message from "./Message";
import ChatSearch from "./ChatSearch";

function ChatBox({ data, groupName, container }) {
    const [offcanvas, setOffcanvas] = useState(false);
    return (
        <div className="flex-grow flex h-screen bg-cream">
            <div className={classNames("transition-all duration-150 ease-in-out", offcanvas ? "w-8/12" : "w-full")}>
                <div className="flex items-center bg-white w-full pl-5 h-[80px] shadow-md sticky top-0">
                    <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="" />
                    <div className="ml-4">
                        <h2 className="text-[20px] font-bold">{groupName}</h2>
                        <p className="text-gray-400">Online</p>
                    </div>
                    <SearchIcon className="w-8 h-8 ml-auto mr-4 text-gray-400 cursor-pointer" onClick={() => setOffcanvas(true)} />
                </div>
                <div className="space-y-6 flex flex-col  overflow-y-scroll h-[89%] whitespace-nowrap p-4 scrollbar-thin scrollbar-thumb-gray-400">
                    {data.map(({ id, first_name, last_name, text }) => (
                        <Message key={id} first_name={first_name} last_name={last_name} text={text} />
                    ))}
                </div>
            </div>
            <ChatSearch offcanvas={offcanvas} setOffcanvas={setOffcanvas} container={container}/>
        </div>
    );
}

export default ChatBox;
