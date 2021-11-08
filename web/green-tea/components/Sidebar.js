import {
    ChartPieIcon,
    ChatAlt2Icon,
    LogoutIcon,
    MenuIcon,
    ViewGridIcon,
} from "@heroicons/react/outline";
import classNames from "classnames";
import Link from "next/link";
import { useState } from "react";

function Sidebar() {
    const [click, setClick] = useState(false);

    const toggleMenu = () => {
        setClick(!click);
    };

    return (
        <div
            className={classNames(
                "relative bg-dark-green  px-4 py-5 h-screen top-0 left-0 transition-all duration-150 ease-in-out",
                click ? "w-[275px]" : "w-[90px]"
            )}
        >
            <div className="flex items-center h-[40px]">
                <h2
                    className={classNames(
                        "text-[30px] font-sans font-semibold text-white ml-4",
                        click ? "block" : "hidden"
                    )}
                >
                    GreenTea
                </h2>
                <MenuIcon
                    className={classNames(
                        "w-[30px] h-[30px] text-white cursor-pointer",
                        click ? "ml-auto" : "ml-3"
                    )}
                    onClick={toggleMenu}
                />
            </div>
            <div className="absolute right-4 left-4">
                <ul className="whitespace-nowrap space-y-4 mt-10">
                    <Link href="/">
                        <li className="flex items-center cursor-pointer px-3 py-2 h-[50px] text-white hover:text-dark-green hover:bg-white hover:rounded-lg">
                            <ViewGridIcon className="w-[30px] h-[30px]" />
                            <a
                                className={classNames(
                                    "text-[20px] ml-4",
                                    click ? "block" : "hidden"
                                )}
                            >
                                Dashboard
                            </a>
                        </li>
                    </Link>

                    <li className="flex items-center cursor-pointer px-3 py-2 h-[50px] text-white hover:text-dark-green hover:bg-white hover:rounded-lg ">
                        <ChartPieIcon className="w-[30px] h-[30px]" />
                        <a
                            className={classNames(
                                "text-[20px] ml-4",
                                click ? "block" : "hidden"
                            )}
                        >
                            Statistic
                        </a>
                    </li>
                    <Link href="/chatlog">
                        <li className="flex items-center cursor-pointer px-3 py-2 h-[50px] text-white hover:text-dark-green hover:bg-white hover:rounded-lg ">
                            <ChatAlt2Icon className="w-[30px] h-[30px]" />

                            <a
                                className={classNames(
                                    "text-[20px] ml-4",
                                    click ? "block" : "hidden"
                                )}
                            >
                                Chat Log
                            </a>
                        </li>
                    </Link>
                </ul>
            </div>

            <div className="absolute bottom-0 left-0 flex items-center border-t-2 w-full z-50 py-2 px-5 bg-[#32502E] whitespace-nowrap">
                <img
                    className={classNames(
                        "h-[40px] w-[40px] rounded-full mr-6",
                        click ? "inline-block" : "hidden"
                    )}
                    src="/profile.jpeg"
                    alt=""
                />
                <p
                    className={classNames(
                        "text-white font-semibold",
                        click ? "inline-block" : "hidden"
                    )}
                >
                    Admin
                </p>
                <div className="hover:bg-opacity-20 hover:bg-white hover:rounded-full p-2 ml-auto">
                    <LogoutIcon className="w-[30px] h-[30px] text-white cursor-pointer" />
                </div>
            </div>
        </div>
    );
}

export default Sidebar;
